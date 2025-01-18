#include "isr.hpp"

#include "arch/intrin.hpp"
#include "util/bits.hpp"
#include "util/digit.hpp"

#include "log.hpp"

#include <stdint.h>
#include <stddef.h>

using sm::uint48_t;

namespace x64 {
    namespace idt {
        static constexpr uint8_t kFlagPresent = (1 << 7);
        static constexpr uint8_t kInterruptGate = 0b1110;
        // static constexpr uint8_t kTrapGate = 0b1111;
        // static constexpr uint8_t kTaskGate = 0b0101;
    }

    struct [[gnu::packed]] IdtEntry {
        uint16_t address0;
        uint16_t selector;
        uint8_t ist;
        uint8_t flags;
        uint48_t address1;
        uint32_t reserved;
    };

    static_assert(sizeof(IdtEntry) == 16);
}

struct alignas(16) Idt {
    static constexpr size_t kCount = 256;
    x64::IdtEntry entries[kCount];
};

static constexpr x64::IdtEntry KmCreateIdtEntry(uintptr_t handler, uint16_t codeSelector, uint8_t dpl, uint8_t ist) {
    uint8_t flags = x64::idt::kFlagPresent | x64::idt::kInterruptGate | ((dpl & 0b11) << 5);

    return x64::IdtEntry {
        .address0 = uint16_t(handler & 0xFFFF),
        .selector = codeSelector,
        .ist = ist,
        .flags = flags,
        .address1 = uint48_t((handler >> 16) & 0xFFFF'FFFF'FFFF),
    };
}

static constexpr size_t kIsrTableStride = 16;
extern "C" char KmIsrTable[];

static Idt gIdt;
static KmIsrHandler KmIsrHandlers[256];

static void *KmDefaultIsrHandler(km::IsrContext *context) {
    KmDebugMessage("[INT] Unhandled interrupt: ", context->vector, " Error: ", context->error, "\n");
    return context;
}

extern "C" void *KmIsrDispatchRoutine(km::IsrContext *context) {
    // Did this interrupt happen while in kernel space?
    bool kernelSpaceInt = (GDT_64BIT_CODE * 0x8) == context->cs;

    // If it didn't then the GS_BASE and KERNEL_GS_BASE registers need to be swapped
    if (!kernelSpaceInt) {
        __swapgs();
    }

    // Then invoke the interrupt handler routine
    void *result = KmIsrHandlers[context->vector](context);

    // And then swapped back again to avoid breaking userspace
    if (!kernelSpaceInt) {
        __swapgs();
    }

    return result;
}

KmIsrHandler KmInstallIsrHandler(uint8_t isr, KmIsrHandler handler) {
    KmIsrHandler old = KmIsrHandlers[isr];
    KmIsrHandlers[isr] = handler;
    return old;
}

void KmUpdateIdtEntry(uint8_t isr, uint16_t selector, uint8_t dpl, uint8_t ist) {
    gIdt.entries[isr] = KmCreateIdtEntry((uintptr_t)KmIsrTable + (isr * kIsrTableStride), selector * 0x8, dpl, ist);
}

void KmInitInterrupts(km::IsrAllocator& isrs, uint16_t codeSelector) {
    for (size_t i = 0; i < Idt::kCount; i++) {
        KmUpdateIdtEntry(i, codeSelector, 0, 0);
    }

    for (size_t i = 0; i < Idt::kCount; i++) {
        KmIsrHandlers[i] = KmDefaultIsrHandler;
    }

    // claim all the system interrupts
    for (uint8_t i = 0; i < 32; i++) {
        isrs.claimIsr(i);
    }

    KmLoadIdt();
}

void KmLoadIdt(void) {
    IDTR idtr = {
        .limit = (sizeof(x64::IdtEntry) * Idt::kCount) - 1,
        .base = (uintptr_t)&gIdt,
    };

    __lidt(idtr);
}

void km::DisableInterrupts() {
    __cli();
}

void km::EnableInterrupts() {
    __sti();
}

void km::IsrAllocator::claimIsr(uint8_t isr) {
    sm::BitsSetBit(mFreeIsrs, sm::BitCount(isr));
}

void km::IsrAllocator::releaseIsr(uint8_t isr) {
    sm::BitsClearBit(mFreeIsrs, sm::BitCount(isr));
}

uint8_t km::IsrAllocator::allocateIsr() {
    sm::BitCount bit = sm::BitsFindAndSetNextFree(mFreeIsrs, sm::BitCount { 0 }, sm::BitCount { kIsrCount });

    return bit.count;
}
