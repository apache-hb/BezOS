#include "apic.hpp"

#include "kernel.hpp"

static constexpr uint16_t kCommandMasterPort = 0x20;
static constexpr uint16_t kDataMasterPort = 0x21;
static constexpr uint16_t kCommandSlavePort = 0xA0;
static constexpr uint16_t kDataSlavePort = 0xA1;

static constexpr uint8_t kInitStart = 0x11;
static constexpr uint8_t kMasterIdtStart = 0x20;
static constexpr uint8_t kSlaveIdtStart = 0x28;
static constexpr uint8_t kSlavePin = 0x2;
static constexpr uint8_t kSlaveId = 0x4;

static constexpr uint32_t kApicBaseMsr = 0x1B;

static constexpr uint64_t kApicAddressMask = 0xFFFFFFFFFFFFF000;
static constexpr uint64_t kApicEnable = (1 << 11);

void KmDisablePIC(void) {
    // start init sequence
    __outbyte(kCommandMasterPort, kInitStart);
    __outbyte(kCommandSlavePort, kInitStart);

    // set interrupt offsets
    __outbyte(kDataMasterPort, kMasterIdtStart);
    __outbyte(kDataSlavePort, kSlaveIdtStart);

    // set master/slave pin
    __outbyte(kDataMasterPort, kSlavePin);
    __outbyte(kDataSlavePort, kSlaveId);

    // set 8086 mode
    __outbyte(kDataMasterPort, 0x1);
    __outbyte(kDataSlavePort, 0x1);

    // mask all interrupts
    __outbyte(kDataMasterPort, 0xFF);
    __outbyte(kDataSlavePort, 0xFF);
}

km::LocalAPIC KmGetLocalAPIC(km::VirtualAllocator& vmm, const km::PageManager& pm) {
    uint64_t msr = __rdmsr(kApicBaseMsr);
    uintptr_t base = (msr & kApicAddressMask);
    bool enabled = msr & kApicEnable;
    KmDebugMessage("[INIT] APIC MSR: ", km::Hex(msr), ", Base address: ", km::Hex(base), ", State: ", km::enabled(enabled), "\n");

    // map the APIC base into the higher half
    km::VirtualAddress vbase = km::VirtualAddress { base + pm.hhdmOffset() };
    vmm.map4k(km::PhysicalAddress { base }, vbase, km::PageFlags::eData);

    return km::LocalAPIC { vbase };
}