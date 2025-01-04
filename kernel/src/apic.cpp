#include "apic.hpp"

#include "delay.hpp"
#include "kernel.hpp"
#include "arch/msr.hpp"

static constexpr uint16_t kCommandMasterPort = 0x20;
static constexpr uint16_t kDataMasterPort = 0x21;
static constexpr uint16_t kCommandSlavePort = 0xA0;
static constexpr uint16_t kDataSlavePort = 0xA1;

static constexpr uint8_t kInitStart = 0x11;
static constexpr uint8_t kMasterIdtStart = 0x20;
static constexpr uint8_t kSlaveIdtStart = 0x28;
static constexpr uint8_t kSlavePin = 0x2;
static constexpr uint8_t kSlaveId = 0x4;

static constexpr x64::ModelRegister<0x1b, x64::RegisterAccess::eReadWrite> kApicBaseMsr;

static constexpr uint64_t kApicAddressMask = 0xFFFFFFFFFFFFF000;
static constexpr uint64_t kApicEnable = (1 << 11);
static constexpr uint64_t kApicBsp = (1 << 8);

static void Disable8259Pic() {
    KmDebugMessage("[INIT] Disabling 8259 PIC.\n");

    // start init sequence
    KmWriteByte(kCommandMasterPort, kInitStart);
    KmWriteByte(kCommandSlavePort, kInitStart);

    // set interrupt offsets
    KmWriteByte(kDataMasterPort, kMasterIdtStart);
    KmWriteByte(kDataSlavePort, kSlaveIdtStart);

    // set master/slave pin
    KmWriteByte(kDataMasterPort, kSlavePin);
    KmWriteByte(kDataSlavePort, kSlaveId);

    // set 8086 mode
    KmWriteByte(kDataMasterPort, 0x1);
    KmWriteByte(kDataSlavePort, 0x1);

    // mask all interrupts
    KmWriteByte(kDataMasterPort, 0xFF);
    KmWriteByte(kDataSlavePort, 0xFF);

    KmDebugMessage("[INIT] PIC disabled\n");
}

// local apic setup

static void LogApicStartup(uint64_t msr) {
    using namespace stdx::literals;

    uintptr_t base = msr & kApicAddressMask;
    bool enabled = msr & kApicEnable;
    bool bsp = msr & kApicBsp;

    stdx::StringView kind = bsp ? "Bootstrap Processor"_sv : "Application Processor"_sv;

    KmDebugMessage("[APIC] Startup: ", km::Hex(msr), ", Base address: ", km::Hex(base), ", State: ", km::enabled(enabled), ", Type: ", kind, "\n");
}

static uint64_t EnableLocalApic(km::PhysicalAddress baseAddress = 0uz) {
    uint64_t msr = kApicBaseMsr.load();

    kApicBaseMsr.update(msr, [&](uint64_t& value) {
        value |= kApicEnable;

        if (baseAddress != 0uz) {
            value &= ~kApicAddressMask;
            value |= baseAddress.address;
        }
    });

    LogApicStartup(msr);

    return msr;
}

static km::LocalApic MapLocalApic(uint64_t msr, km::SystemMemory& memory) {
    KM_CHECK(msr & kApicEnable, "APIC not enabled");

    uintptr_t base = msr & kApicAddressMask;

    // Intel SDM Vol 3A 12-4 12.4.1
    // For correct APIC operation, this address space must
    // be mapped to an area of memory that has
    // been designated as strong uncacheable (UC)
    void *addr = memory.hhdmMap(base, base + km::kApicSize, km::PageFlags::eData, km::MemoryType::eUncached);

    return km::LocalApic { addr };
}

volatile uint32_t& km::LocalApic::reg(uint16_t offset) const {
    return *reinterpret_cast<volatile uint32_t*>((char*)mBaseAddress + offset);
}

km::LocalApic km::LocalApic::current(km::SystemMemory& memory) {
    uint64_t reg = kApicBaseMsr.load();

    return MapLocalApic(reg, memory);
}

km::LocalApic KmInitBspLocalApic(km::SystemMemory& memory) {
    // Disable the emulated 8259 PIC before starting up the local apic.
    Disable8259Pic();

    uint64_t msr = EnableLocalApic();

    return MapLocalApic(msr, memory);
}

km::LocalApic KmInitApLocalApic(km::SystemMemory& memory, km::LocalApic& bsp) {
    // Remap every AP lapic at the same address as the BSP lapic.
    km::PhysicalAddress bspBaseAddress = uintptr_t(bsp.baseAddress()) - memory.pager.hhdmOffset();
    uint64_t msr = EnableLocalApic(bspBaseAddress);

    return MapLocalApic(msr, memory);
}

// local apic methods

uint32_t km::LocalApic::id() const {
    uint32_t id = reg(kApicId);
    return id >> 24;
}

uint32_t km::LocalApic::version() const {
    return reg(kApicVersion) & 0xFF;
}

// io apic

static constexpr uint32_t kIoApicSelect = 0x0;
static constexpr uint32_t kIoApicWindow = 0x10;

static constexpr uint32_t kIoApicVersion = 0x1;

static uint8_t *hhdmMapIoApic(km::SystemMemory& memory, const acpi::MadtEntry *entry) {
    const acpi::MadtEntry::IoApic ioApic = entry->ioapic;
    return (uint8_t*)memory.hhdmMap(ioApic.address, ioApic.address + entry->length);
}

km::IoApic::IoApic(const acpi::MadtEntry *entry, km::SystemMemory& memory)
    : mAddress(hhdmMapIoApic(memory, entry))
    , mIsrBase(entry->ioapic.interruptBase)
    , mId(entry->ioapic.ioApicId)
{ }

volatile uint32_t& km::IoApic::reg(uint32_t offset) {
    return *(volatile uint32_t*)(mAddress + offset);
}

void km::IoApic::select(uint32_t field) {
    reg(kIoApicSelect) = field;
}

uint32_t km::IoApic::read(uint32_t field) {
    select(field);
    return reg(kIoApicWindow);
}

uint16_t km::IoApic::inputCount() {
    uint32_t version = read(kIoApicVersion);
    return ((version >> 16) & 0xFF) + 1;
}

uint8_t km::IoApic::version() {
    return read(kIoApicVersion) & 0xFF;
}
