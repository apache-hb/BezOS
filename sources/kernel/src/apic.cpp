#include "apic.hpp"

#include "memory/address_space.hpp"
#include "arch/msr.hpp"

#include "delay.hpp"
#include "log.hpp"
#include "panic.hpp"

#include "util/cpuid.hpp"

static constexpr x64::RwModelRegister<0x1b> IA32_APIC_BASE;

static constexpr uint16_t kX2ApicBaseMsr = 0x800;

static constexpr x64::RoModelRegister<kX2ApicBaseMsr + 0x2> IA32_X2APIC_ID;
static constexpr x64::WoModelRegister<kX2ApicBaseMsr + 0x30> IA32_X2APIC_ICR;
static constexpr x64::WoModelRegister<kX2ApicBaseMsr + 0x3f> IA32_X2APIC_SELF_IPI;

static constexpr uint64_t kApicAddressMask = 0xFFFFFFFFFFFFF000;
static constexpr uint64_t kApicEnableBit = (1 << 11);
static constexpr uint32_t kX2ApicEnableBit = (1 << 10);
static constexpr uint64_t kApicBspBit = (1 << 8);

static constexpr uint32_t kApicSoftwareEnable = (1 << 8);

void km::Disable8259Pic() {
    static constexpr uint16_t kCommandMasterPort = 0x20;
    static constexpr uint16_t kDataMasterPort = 0x21;
    static constexpr uint16_t kCommandSlavePort = 0xA0;
    static constexpr uint16_t kDataSlavePort = 0xA1;

    static constexpr uint8_t kInitStart = 0x11;
    static constexpr uint8_t kMasterIdtStart = 0x20;
    static constexpr uint8_t kSlaveIdtStart = 0x28;
    static constexpr uint8_t kSlavePin = 0x2;
    static constexpr uint8_t kSlaveId = 0x4;

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

// apic setup

static void LogApicStartup(uint64_t msr) {
    using namespace stdx::literals;

    bool enabled = msr & kApicEnableBit;
    bool x2apic = msr & kX2ApicEnableBit;
    bool bsp = msr & kApicBspBit;

    stdx::StringView kind = bsp ? "Bootstrap Processor"_sv : "Application Processor"_sv;

    if (x2apic) {
        KmDebugMessage("[APIC] Startup: ", km::Hex(msr), ", State: x2APIC enabled, Type: ", kind, "\n");
    } else {
        km::PhysicalAddress base = msr & kApicAddressMask;
        KmDebugMessage("[APIC] Startup: ", km::Hex(msr), ", Base address: ", base, ", State: ", km::enabled(enabled), ", Type: ", kind, "\n");
    }
}

// x2apic free functions

void km::EnableX2Apic() {
    //
    // Intel® 64 Architecture x2APIC Specification
    // Table 2-1. x2APIC Operating Mode Configurations
    // Both IA32_APIC_BASE[11] (xAPIC global enable) and IA32_APIC_BASE[10] (x2APIC enable) must be set.
    //
    IA32_APIC_BASE |= (kX2ApicEnableBit | kApicEnableBit);

    LogApicStartup(IA32_APIC_BASE.load());
}

bool km::HasX2ApicSupport() {
    sm::CpuId cpuid = sm::CpuId::of(1);
    return cpuid.ecx & (1 << 21);
}

bool km::IsX2ApicEnabled() {
    return IA32_APIC_BASE.load() & kX2ApicEnableBit;
}

// x2apic methods

uint64_t km::X2Apic::read(uint16_t offset) const {
    return __rdmsr(kX2ApicBaseMsr + offset);
}

void km::X2Apic::write(uint16_t offset, uint64_t value) noexcept [[clang::reentrant]] {
    __wrmsr(kX2ApicBaseMsr + offset, value);
}

uint32_t km::X2Apic::id() const {
    return IA32_X2APIC_ID.load();
}

void km::X2Apic::selfIpi(uint8_t vector) {
    IA32_X2APIC_SELF_IPI = vector;
}

void km::X2Apic::writeIcr(uint32_t dst, uint32_t cmd) {
    uint64_t icr = uint64_t(dst) << 32 | cmd;
    IA32_X2APIC_ICR = icr;
}

// local apic free functions

static km::LocalApic MapLocalApic(uint64_t msr, km::AddressSpace& memory) {
    km::TlsfAllocation allocation;
    static constexpr size_t kApicSize = 0x400;

    KM_CHECK(msr & kApicEnableBit, "APIC not enabled");
    KM_CHECK(!(msr & kX2ApicEnableBit), "Cannot use local APIC in x2APIC mode");

    uintptr_t base = msr & kApicAddressMask;

    //
    // Intel SDM Vol 3A 12-4 12.4.1
    // For correct APIC operation, this address space must
    // be mapped to an area of memory that has
    // been designated as strong uncacheable (UC)
    //
    // TODO: make a struct to represent the local apic mmio registers
    void *addr = memory.mapGenericObject(km::MemoryRangeEx::of(base, kApicSize), km::PageFlags::eData, km::MemoryType::eUncached, &allocation);
    KM_CHECK(addr, "Failed to map local APIC");

    return km::LocalApic { allocation, addr };
}

static uint64_t EnableLocalApic(km::PhysicalAddress baseAddress = 0uz) {
    uint64_t msr = IA32_APIC_BASE.load();

    IA32_APIC_BASE.update(msr, [&](uint64_t& value) {
        value |= kApicEnableBit;

        if (baseAddress != 0uz) {
            value &= ~kApicAddressMask;
            value |= baseAddress.address;
        }
    });

    LogApicStartup(msr);

    return msr;
}

// local apic methods

volatile uint32_t& km::LocalApic::reg(uint16_t offset) const {
    return *std::bit_cast<volatile uint32_t*>(mAddress + offset);
}

uint64_t km::LocalApic::read(uint16_t offset) const {
    return reg(offset << 4);
}

void km::LocalApic::write(uint16_t offset, uint64_t value) noexcept [[clang::reentrant]] {
    reg(offset << 4) = value;
}

uint32_t km::LocalApic::id() const {
    uint32_t id = reg(kApicId);
    return id >> 24;
}

void km::LocalApic::selfIpi(uint8_t vector) {
    IApic::sendIpi(apic::IcrDeliver::eSelf, apic::IpiAlert { vector });
}

bool km::LocalApic::pendingIpi() {
    static constexpr uint32_t kDeliveryStatus = (1 << 12);
    return read(apic::kIcr0) & kDeliveryStatus;
}

void km::LocalApic::writeIcr(uint32_t dst, uint32_t cmd) {
    reg(kIcr1) = dst << 24;
    reg(kIcr0) = cmd;
}

// generic apic methods

static constexpr uint32_t BuildIpi(km::apic::IpiAlert alert) {
    uint32_t result = 0;
    result |= alert.vector;
    result |= (std::to_underlying(alert.mode) << 8);
    result |= (std::to_underlying(alert.dst) << 11);
    result |= (std::to_underlying(alert.level) << 14);
    result |= (std::to_underlying(alert.trigger) << 15);
    return result;
}

static constexpr uint32_t BuildIpiShorthand(km::apic::IpiAlert alert, km::apic::IcrDeliver deliver) {
    uint32_t result = BuildIpi(alert);
    result |= (std::to_underlying(deliver) << 18);
    return result;
}

static_assert(BuildIpi(km::apic::IpiAlert::init()) == 0x4500);
static_assert(BuildIpi(km::apic::IpiAlert::sipi(0x8000)) == 0x4608);

void km::IApic::sendIpi(apic::IcrDeliver deliver, uint8_t vector) {
    sendIpi(deliver, km::apic::IpiAlert { vector });
}

void km::IApic::sendIpi(uint32_t dst, apic::IpiAlert alert) {
    writeIcr(dst, BuildIpi(alert));
}

void km::IApic::sendIpi(apic::IcrDeliver deliver, apic::IpiAlert alert) {
    writeIcr(0, BuildIpiShorthand(alert, deliver));
}

uint32_t km::IApic::version() const {
    return read(apic::kApicVersion) & 0xFF;
}

void km::IApic::eoi() noexcept [[clang::reentrant]] {
    write(apic::kEndOfInt, 0);
}

void km::IApic::maskTaskPriority() {
    static constexpr uint32_t kMaskTaskPriority = 1 << 4;
    uint32_t value = read(apic::kTaskPriority);
    value |= kMaskTaskPriority;
    write(apic::kTaskPriority, value);
}

void km::IApic::maskIvt(apic::Ivt ivt) {
    static constexpr uint64_t kMaskBit = 1 << 16;
    uint64_t value = read(std::to_underlying(ivt));
    value |= kMaskBit;
    write(std::to_underlying(ivt), value);
}

km::apic::ErrorState km::IApic::status() {
    uint32_t esr = read(apic::kErrorStatus);

    return apic::ErrorState {
        .egressChecksum = bool(esr & (1 << 0)),
        .ingressChecksum = bool(esr & (1 << 1)),
        .egressAccept = bool(esr & (1 << 2)),
        .ingressAccept = bool(esr & (1 << 3)),
        .priorityIpi = bool(esr & (1 << 4)),
        .egressVector = bool(esr & (1 << 5)),
        .ingressVector = bool(esr & (1 << 6)),
        .illegalRegister = bool(esr & (1 << 7)),
    };
}

void km::IApic::configure(apic::Ivt ivt, apic::IvtConfig config) {
    uint32_t entry
        = config.vector
        | (std::to_underlying(config.polarity) << 13)
        | (std::to_underlying(config.trigger) << 15)
        | ((config.enabled ? 0 : 1) << 16);

    if (ivt == apic::Ivt::eTimer && config.timer != apic::TimerMode::eNone) {
        entry |= (std::to_underlying(config.timer) << 17);
    }

    write(std::to_underlying(ivt), entry);
}

void km::IApic::setTimerDivisor(apic::TimerDivide timer) {
    auto v = std::to_underlying(timer);
    uint32_t msr = (v & 0b11) | ((v & 0b100) << 1);
    write(apic::kDivide, msr);
}

void km::IApic::setInitialCount(uint64_t count) {
    write(apic::kInitialCount, count);
}

uint64_t km::IApic::getCurrentCount() {
    return read(apic::kCurrentCount);
}

void km::IApic::enableSpuriousInt() {
    uint32_t value = read(apic::kSpuriousInt);
    value |= kApicSoftwareEnable;
    write(apic::kSpuriousInt, value);
}

void km::IApic::enable() {
    maskTaskPriority();
    enableSpuriousInt();
}

void km::IApic::setSpuriousVector(uint8_t vector) {
    uint32_t value = read(apic::kSpuriousInt);
    value = (value & ~0xFF) | vector;
    write(apic::kSpuriousInt, value);
}

// generic apic free functions

km::Apic km::InitBspApic(km::AddressSpace& memory, bool useX2Apic) {
    if (useX2Apic) {
        km::EnableX2Apic();
        return km::X2Apic::get();
    } else {
        uint64_t msr = EnableLocalApic();
        return MapLocalApic(msr, memory);
    }
}

km::Apic km::InitApApic(km::AddressSpace& memory, const km::IApic *bsp) {
    if (bsp->type() == apic::Type::eX2Apic) {
        km::EnableX2Apic();

        return km::X2Apic::get();
    } else {
        const km::LocalApic *lapic = static_cast<const km::LocalApic*>(bsp);
        sm::VirtualAddress vaddr = lapic->baseAddress();

        // Move every AP lapic at the same address as the BSP lapic.
        // This lets us reuse the same virtual address space allocation for all APs.
        km::PhysicalAddress bspBaseAddress = memory.getBackingAddress(vaddr);
        EnableLocalApic(bspBaseAddress);

        return auto{*lapic};
    }
}

// io apic

static constexpr uint32_t kIoApicId = 0x0;
static constexpr uint32_t kIoApicVersion = 0x1;
static constexpr uint32_t kIoApicArbitration = 0x2;

static sm::VirtualAddressOf<km::detail::IoApicMmio> mapIoApic(km::AddressSpace& memory, acpi::MadtEntry::IoApic entry, km::TlsfAllocation *allocation) {
    return memory.mapMmio<km::detail::IoApicMmio>(entry.address, km::PageFlags::eData, allocation);
}

km::IoApic::IoApic(const acpi::MadtEntry *entry, km::AddressSpace& memory)
    : IoApic(entry->ioapic, memory)
{ }

km::IoApic::IoApic(acpi::MadtEntry::IoApic entry, km::AddressSpace& memory)
    : mMmio(mapIoApic(memory, entry, &mAllocation))
{
    uint32_t idreg = read(kIoApicId);

    //
    // Extract the IOAPIC ID from the ID register.
    // The ID is in bits 24-27.
    //
    uint8_t ioapicId = (idreg & (0b111 << 24)) >> 24;
    uint8_t acpiId = entry.ioApicId;

    if (ioapicId != acpiId) {
        KmDebugMessage("[APIC] IOAPIC ID mismatch between acpi tables and hardware: ", ioapicId, " != ", acpiId, "\n");
        KmDebugMessage("[APIC] Prefering IOAPIC ID reported by hardware: ", ioapicId, "\n");
    }

    mId = ioapicId;
    mIsrBase = entry.interruptBase;
}

void km::IoApic::select(uint32_t field) {
    mMmio->ioregsel = field;
}

uint32_t km::IoApic::read(uint32_t field) {
    select(field);
    return mMmio->iowin;
}

void km::IoApic::write(uint32_t field, uint32_t value) {
    select(field);
    mMmio->iowin = value;
}

uint16_t km::IoApic::inputCount() {
    uint32_t version = read(kIoApicVersion);
    return ((version >> 16) & 0xFF) + 1;
}

uint8_t km::IoApic::version() {
    return read(kIoApicVersion) & 0xFF;
}

uint8_t km::IoApic::arbitrationId() {
    uint32_t arb = read(kIoApicArbitration);
    return (arb >> 24) & 0x0F;
}

static constexpr uint64_t kActiveLow = (1 << 13);
static constexpr uint64_t kLevel = (1 << 15);
static constexpr uint64_t kMasked = (1 << 16);

void km::IoApic::setRedirect(apic::IvtConfig config, uint32_t redirect, const IApic *target) {
    uint64_t entry = config.vector | uint64_t(target->id()) << 56;

    if (config.polarity == apic::Polarity::eActiveLow) {
        entry |= kActiveLow;
    }

    if (config.trigger == apic::Trigger::eLevel) {
        entry |= kLevel;
    }

    if (!config.enabled) {
        entry |= kMasked;
    }

    uint32_t ioredirect = (redirect - mIsrBase) * 2 + 0x10;
    write(ioredirect + 0, entry & UINT32_MAX);
    write(ioredirect + 1, entry >> 32);
}

static km::apic::Trigger GetIsoTriggerMode(acpi::MadtEntry::InterruptSourceOverride iso) {
    switch (iso.flags & 0b1100) {
    case 0b0000:
    case 0b0100:
        return km::apic::Trigger::eEdge;
    case 0b1100:
        return km::apic::Trigger::eLevel;

    default:
        KmDebugMessage("[WARN] Unknown trigger mode: ", km::Hex(iso.flags & 0b1100), "\n");
        return km::apic::Trigger::eEdge;
    }
}

static km::apic::Polarity GetIsoPolarity(acpi::MadtEntry::InterruptSourceOverride iso) {
    switch (iso.flags & 0b11) {
    case 0b00:
    case 0b11:
        return km::apic::Polarity::eActiveLow;
    case 0b01:
        return km::apic::Polarity::eActiveHigh;

    default:
        KmDebugMessage("[WARN] Unknown polarity mode: ", km::Hex(iso.flags & 0b11), "\n");
        return km::apic::Polarity::eActiveLow;
    }
}

static bool IoApicContainsGsi(km::IoApic& ioApic, uint32_t gsi) {
    uint32_t first = ioApic.isrBase();
    uint32_t last = first + ioApic.inputCount() - 1;
    return gsi >= first && gsi <= last;
}

km::IoApicSet::IoApicSet(const acpi::Madt *madt, km::AddressSpace& memory)
    : mMadt(madt)
    , mIoApics(mMadt->ioApicCount())
{
    size_t index = 0;
    for (const acpi::MadtEntry *entry : *mMadt) {
        if (entry->type != acpi::MadtEntryType::eIoApic)
            continue;

        IoApic ioapic { entry, memory };

        KmDebugMessage("[INIT] IOAPIC ", index, " ID: ", ioapic.id(), ", Version: ", ioapic.version(), "\n");
        KmDebugMessage("[INIT] ISR base: ", ioapic.isrBase(), ", Inputs: ", ioapic.inputCount(), "\n");

        mIoApics[index++] = ioapic;
    }
}

void km::IoApicSet::setRedirect(apic::IvtConfig config, uint32_t redirect, const IApic *target) {
    for (IoApic& ioApic : mIoApics) {
        if (IoApicContainsGsi(ioApic, redirect)) {
            ioApic.setRedirect(config, redirect, target);
            return;
        }
    }

    KmDebugMessage("[WARN] GSI ", redirect, " not found in any IOAPIC\n");
}

void km::IoApicSet::setLegacyRedirect(apic::IvtConfig config, uint32_t redirect, const IApic *target) {
    for (const acpi::MadtEntry *entry : *mMadt) {
        if (entry->type != acpi::MadtEntryType::eInterruptSourceOverride)
            continue;

        const acpi::MadtEntry::InterruptSourceOverride iso = entry->iso;
        if (iso.source != redirect)
            continue;

        apic::IvtConfig fixup {
            .vector = uint8_t(config.vector),
            .polarity = GetIsoPolarity(iso),
            .trigger = GetIsoTriggerMode(iso),
            .enabled = true,
        };

        uint32_t isoGsi = iso.interrupt;

        KmDebugMessage("[INIT] IRQ PIN ", redirect, " remapped to PIN ", isoGsi, " by MADT\n");

        setRedirect(fixup, isoGsi, target);

        return;
    }

    KmDebugMessage("[INIT] IRQ PIN ", redirect, " redirected to APIC ", target->id(), ":", config.vector, "\n");

    setRedirect(config, redirect, target);
}

using EsrFormat = km::Format<km::apic::ErrorState>;

void EsrFormat::format(km::IOutStream& out, km::apic::ErrorState esr) {
    out.format("ESR(");
    bool first = true;
    auto append = [&](bool value, stdx::StringView name) {
        if (value) {
            if (!first)
                out.format(", ");
            first = false;
            out.format(name);
        }
    };

    append(esr.egressChecksum, "EgressChecksum");
    append(esr.ingressChecksum, "IngressChecksum");
    append(esr.egressAccept, "EgressAccept");
    append(esr.ingressAccept, "IngressAccept");
    append(esr.priorityIpi, "PriorityIpi");
    append(esr.egressVector, "EgressVector");
    append(esr.ingressVector, "IngressVector");
    append(esr.illegalRegister, "IllegalRegister");

    out.format(")");
}
