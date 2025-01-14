#include "isr.hpp"

#include "arch/intrin.hpp"
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

        static constexpr uint8_t kPrivilegeRing0 = 0;
        // static constexpr uint8_t kPrivilegeRing1 = 1;
        // static constexpr uint8_t kPrivilegeRing2 = 2;
        // static constexpr uint8_t kPrivilegeRing3 = 3;
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

static x64::IdtEntry KmCreateIdtEntry(void *handler, uint16_t codeSelector, uint8_t dpl) {
    uint8_t flags = x64::idt::kFlagPresent | x64::idt::kInterruptGate | ((dpl & 0b11) << 5);
    uintptr_t address = (uintptr_t)handler;

    return x64::IdtEntry {
        .address0 = uint16_t(address & 0xFFFF),
        .selector = codeSelector,
        .flags = flags,
        .address1 = uint48_t((address >> 16) & 0xFFFF'FFFF'FFFF),
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
    return KmIsrHandlers[context->vector](context);
}

KmIsrHandler KmInstallIsrHandler(uint8_t isr, KmIsrHandler handler) {
    KmIsrHandler old = KmIsrHandlers[isr];
    KmIsrHandlers[isr] = handler;
    return old;
}

void KmInitInterrupts(km::IsrAllocator& isrs, uint16_t codeSelector) {
    for (size_t i = 0; i < Idt::kCount; i++) {
        gIdt.entries[i] = KmCreateIdtEntry(KmIsrTable + (i * kIsrTableStride), codeSelector, x64::idt::kPrivilegeRing0);
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

void km::IsrAllocator::claimIsr(uint8_t isr) {
    mFreeIsrs[isr / CHAR_BIT] |= (1 << (isr % CHAR_BIT));
}

void km::IsrAllocator::releaseIsr(uint8_t isr) {
    mFreeIsrs[isr / CHAR_BIT] &= ~(1 << (isr % CHAR_BIT));
}

uint8_t km::IsrAllocator::allocateIsr() {
    for (size_t i = 0; i < kIsrCount / CHAR_BIT; i++) {
        if (mFreeIsrs[i] != 0xFF) {
            for (size_t j = 0; j < CHAR_BIT; j++) {
                if (!(mFreeIsrs[i] & (1 << j))) {
                    mFreeIsrs[i] |= (1 << j);
                    return i * CHAR_BIT + j;
                }
            }
        }
    }

    return 0;
}
