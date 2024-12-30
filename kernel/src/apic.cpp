#include "apic.hpp"

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

static constexpr x64::ModelRegister<0x1b, x64::RegisterAccess::eRead> kApicBaseMsr;

static constexpr uint64_t kApicAddressMask = 0xFFFFFFFFFFFFF000;
static constexpr uint64_t kApicEnable = (1 << 11);
static constexpr uint64_t kApicBsp = (1 << 8);

void KmDisablePIC(void) {
    KmDebugMessage("[INIT] Disabling PIC\n");

    // TODO: not writing a delay here because it causes a fault on KVM.
    // On real hardware it's fine, and sometimes even required for correctness.
    // Need a way of detecting platform type to determine if we need to delay or not.

    // start init sequence
    KmWriteByteNoDelay(kCommandMasterPort, kInitStart);
    KmWriteByteNoDelay(kCommandSlavePort, kInitStart);

    // set interrupt offsets
    KmWriteByteNoDelay(kDataMasterPort, kMasterIdtStart);
    KmWriteByteNoDelay(kDataSlavePort, kSlaveIdtStart);

    // set master/slave pin
    KmWriteByteNoDelay(kDataMasterPort, kSlavePin);
    KmWriteByteNoDelay(kDataSlavePort, kSlaveId);

    // set 8086 mode
    KmWriteByteNoDelay(kDataMasterPort, 0x1);
    KmWriteByteNoDelay(kDataSlavePort, 0x1);

    // mask all interrupts
    KmWriteByteNoDelay(kDataMasterPort, 0xFF);
    KmWriteByteNoDelay(kDataSlavePort, 0xFF);

    KmDebugMessage("[INIT] PIC disabled\n");
}

km::LocalAPIC KmInitLocalAPIC(km::VirtualAllocator& vmm, const km::PageManager& pm) {
    uint64_t msr = kApicBaseMsr.load();
    uintptr_t base = (msr & kApicAddressMask);
    bool enabled = msr & kApicEnable;
    bool bsp = msr & kApicBsp;
    KmDebugMessage("[INIT] APIC MSR: ", km::Hex(msr), ", Base address: ", km::Hex(base), ", State: ", km::enabled(enabled), ", BSP: ", bsp ? stdx::StringView("True") : stdx::StringView("False"), "\n");

    // map the APIC base into the higher half
    km::VirtualAddress vbase = km::VirtualAddress { base + pm.hhdmOffset() };
    km::MemoryRange range { base, base + km::kApicSize };
    vmm.mapRange(range, vbase, km::PageFlags::eData);

    return km::LocalAPIC { vbase };
}

static constexpr uint32_t kIoApicSelect = 0x0;
static constexpr uint32_t kIoApicWindow = 0x10;

static constexpr uint32_t kIoApicVersion = 0x1;

km::IoApic::IoApic(const acpi::MadtEntry *entry, km::SystemMemory& memory)
    : mAddress((uint8_t*)memory.hhdmMap(entry->ioapic.address, entry->ioapic.address + entry->length))
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
