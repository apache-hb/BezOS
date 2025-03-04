#include "isr/isr.hpp"

#include "arch/intrin.hpp"

struct alignas(16) Idt {
    static constexpr size_t kCount = 256;
    x64::IdtEntry entries[kCount];
};

static constexpr size_t kIsrTableStride = 16;
extern "C" const char KmIsrTable[];
static constinit Idt gIdt{};

template<typename F>
static km::IsrContext DispatchIsr(km::IsrContext *context, F&& handler) {
    //
    // If this interrupt happened while in userspace then the
    // GS_BASE and KERNEL_GS_BASE registers need to be swapped.
    //
    if ((context->cs & 0b11) != 0) {
        __swapgs();
    }

    //
    // Then invoke the interrupt handler routine.
    //
    km::IsrContext result = handler(context);

    //
    // If the interrupt will be returning to userspace then swap
    // again to restore the GS_BASE register. This is a distinct
    // check from the one above because the interrupt handler may
    // have been a part of scheduling, which can switch between
    // kernel and userspace.
    //
    if ((result.cs & 0b11) != 0) {
        __swapgs();
    }

    return result;
}

extern "C" km::IsrContext KmIsrDispatchRoutine(km::IsrContext *context) {
    return DispatchIsr(context, [](km::IsrContext *context) {
        uint8_t vector = uint8_t(context->vector);
        if (vector < km::isr::kExceptionCount) {
            km::SharedIsrTable *ist = km::GetSharedIsrTable();
            return ist->invoke(context);
        }

        km::LocalIsrTable *ist = km::GetLocalIsrTable();
        return ist->invoke(context);
    });
}

void km::UpdateIdtEntry(uint8_t isr, uint16_t selector, x64::Privilege dpl, uint8_t ist) {
    gIdt.entries[isr] = x64::CreateIdtEntry((uintptr_t)KmIsrTable + (isr * kIsrTableStride), selector * 0x8, dpl, ist);
}

void km::InitInterrupts(uint16_t cs) {
    for (size_t i = 0; i < Idt::kCount; i++) {
        UpdateIdtEntry(i, cs, x64::Privilege::eSupervisor, 0);
    }

    LoadIdt();
}

void km::LoadIdt(void) {
    IDTR idtr = {
        .limit = sizeof(Idt) - 1,
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
