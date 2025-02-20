#include "isr.hpp"

#include "arch/intrin.hpp"
#include "arch/isr.hpp"

#include "thread.hpp"
#include "util/digit.hpp"

#include "gdt.h"
#include "panic.hpp"
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

    struct alignas(16) Idt {
        static constexpr size_t kCount = 256;
        x64::IdtEntry entries[kCount];
    };
}

static constexpr size_t kIsrTableStride = 16;
extern "C" const char KmIsrTable[];
static x64::Idt gIdt;

static constinit km::SharedIsrTable gSharedIsrTable{};

static constinit km::LocalIsrTable *gIsrTable = nullptr;

CPU_LOCAL
static constinit km::CpuLocal<km::LocalIsrTable*> tlsIsrTable;
static constinit bool gEnableLocalIsrTable = false;

static constexpr x64::IdtEntry CreateIdtEntry(uintptr_t handler, uint16_t codeSelector, km::Privilege dpl, uint8_t ist) {
    uint8_t flags = x64::idt::kFlagPresent | x64::idt::kInterruptGate | ((std::to_underlying(dpl) & 0b11) << 5);

    return x64::IdtEntry {
        .address0 = uint16_t(handler & 0xFFFF),
        .selector = codeSelector,
        .ist = ist,
        .flags = flags,
        .address1 = uint48_t((handler >> 16)),
    };
}

km::IsrContext km::DefaultIsrHandler(km::IsrContext *context) {
    KmDebugMessage("[INT] Unhandled interrupt: ", context->vector, " Error: ", context->error, "\n");
    return *context;
}

template<typename F>
static km::IsrContext DispatchIsr(km::IsrContext *context, F&& handler) {
    //
    // Did this interrupt happen while in user space?
    //
    bool userSpaceInt = context->cs & 0b11;

    //
    // If it did then the GS_BASE and KERNEL_GS_BASE registers need to be swapped.
    //
    if (userSpaceInt) {
        __swapgs();
    }

    //
    // Then invoke the interrupt handler routine.
    //
    km::IsrContext result = handler(context);

    //
    // And then swapped back again to avoid breaking userspace.
    //
    if (userSpaceInt) {
        __swapgs();
    }

    return result;
}

extern "C" km::IsrContext KmIsrDispatchRoutine(km::IsrContext *context) {
    return DispatchIsr(context, [](km::IsrContext *context) {
        uint8_t vector = uint8_t(context->vector);
        if (vector < km::isr::kExceptionCount) {
            return gSharedIsrTable.invoke(context);
        }

        km::LocalIsrTable *ist = gEnableLocalIsrTable ? tlsIsrTable.get() : gIsrTable;
        return ist->invoke(context);
    });
}

void km::UpdateIdtEntry(uint8_t isr, uint16_t selector, Privilege dpl, uint8_t ist) {
    gIdt.entries[isr] = CreateIdtEntry((uintptr_t)KmIsrTable + (isr * kIsrTableStride), selector * 0x8, dpl, ist);
}

void km::InitInterrupts(LocalIsrTable *ist, uint16_t codeSelector) {
    gIsrTable = ist;

    for (size_t i = 0; i < x64::Idt::kCount; i++) {
        UpdateIdtEntry(i, codeSelector, Privilege::eSupervisor, 0);
    }

    LoadIdt();
}

void km::LoadIdt(void) {
    IDTR idtr = {
        .limit = (sizeof(x64::IdtEntry) * x64::Idt::kCount) - 1,
        .base = (uintptr_t)&gIdt,
    };

    __lidt(idtr);
}

km::SharedIsrTable *km::GetSharedIsrTable() {
    return &gSharedIsrTable;
}

void km::SetCpuLocalIsrTable(LocalIsrTable *table) {
    tlsIsrTable = table;
}

void km::EnableCpuLocalIsrTable() {
    gEnableLocalIsrTable = true;
}

void km::DisableInterrupts() {
    __cli();
}

void km::EnableInterrupts() {
    __sti();
}
