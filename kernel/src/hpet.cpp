#include "pit.hpp"

struct [[gnu::packed]] HpetComparator {
    volatile uint64_t config;
    volatile uint64_t comparator;
    volatile uint64_t irq;
    uint8_t reserved[8];
};

struct [[gnu::packed]] HpetRegisters {
    volatile const uint64_t id;
    uint8_t reserved0[8];
    volatile uint64_t config;
    uint8_t reserved1[8];
    volatile uint64_t irqStatus;
    uint8_t reserved2[200];
    volatile uint64_t counter;
    uint8_t reserved3[8];
    HpetComparator comparators[32];
};

static_assert(offsetof(HpetRegisters, id) == 0x00);
static_assert(offsetof(HpetRegisters, config) == 0x10);
static_assert(offsetof(HpetRegisters, irqStatus) == 0x20);
static_assert(offsetof(HpetRegisters, counter) == 0xF0);
static_assert(offsetof(HpetRegisters, comparators) == 0x100);

km::HighPrecisionTimer::HighPrecisionTimer(const acpi::Hpet *hpet, SystemMemory& memory)
    : mTable(*hpet)
{
    void *addr = memory.mapObject<HpetRegisters>(km::PhysicalAddress { hpet->baseAddress.address });
}

km::hertz km::HighPrecisionTimer::refclk() const {
    return 0 * si::hertz;
}

uint64_t km::HighPrecisionTimer::ticks() const {
    return 0;
}
