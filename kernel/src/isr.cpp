#include "isr.hpp"

#include "arch/intrin.hpp"
#include "arch/isr.hpp"

#include "thread.hpp"
#include "util/bits.hpp"
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

static constinit km::IsrTable *gIsrTable = nullptr;

CPU_LOCAL
static constinit km::CpuLocal<km::IsrTable*> tlsIsrTable;

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
    // Did this interrupt happen while in kernel space?
    bool kernelSpaceInt = (GDT_64BIT_CODE * 0x8) == context->cs;

    // If it didn't then the GS_BASE and KERNEL_GS_BASE registers need to be swapped
    if (!kernelSpaceInt) {
        __swapgs();
    }

    // Then invoke the interrupt handler routine
    km::IsrContext result = handler(context);

    // And then swapped back again to avoid breaking userspace
    if (!kernelSpaceInt) {
        __swapgs();
    }

    return result;
}

extern "C" km::IsrContext KmIsrDispatchRoutine(km::IsrContext *context) {
    return DispatchIsr(context, [](km::IsrContext *context) {
        return gIsrTable->invoke(context);
    });
}

void km::UpdateIdtEntry(uint8_t isr, uint16_t selector, Privilege dpl, uint8_t ist) {
    gIdt.entries[isr] = CreateIdtEntry((uintptr_t)KmIsrTable + (isr * kIsrTableStride), selector * 0x8, dpl, ist);
}

void km::InitInterrupts(km::IsrAllocator& isrs, IsrTable *ist, uint16_t codeSelector) {
    gIsrTable = ist;

    for (size_t i = 0; i < x64::Idt::kCount; i++) {
        UpdateIdtEntry(i, codeSelector, Privilege::eSupervisor, 0);
    }

    // claim all the system interrupts
    for (uint8_t i = 0; i < 32; i++) {
        isrs.claimIsr(i);
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

void km::DisableInterrupts() {
    __cli();
}

void km::EnableInterrupts() {
    __sti();
}

uint8_t km::IsrAllocator::claimIsr(uint8_t isr) {
    sm::BitsSetBit(mFreeIsrs, sm::BitCount(isr));
    return isr;
}

void km::IsrAllocator::releaseIsr(uint8_t isr) {
    sm::BitsClearBit(mFreeIsrs, sm::BitCount(isr));
}

uint8_t km::IsrAllocator::allocateIsr() {
    sm::BitCount bit = sm::BitsFindAndSetNextFree(mFreeIsrs, sm::BitCount { 0 }, sm::BitCount { kIsrCount });

    return bit.count;
}


km::IsrTable::Entry *km::IsrTable::find(const Entry *handle) {
    //
    // We need to be very certain what we're about do is alright.
    //
    if (std::begin(mHandlers) > handle || handle >= std::end(mHandlers)) {
        KmDebugMessage("The isr table (", (void*)std::begin(mHandlers), "-", (void*)std::end(mHandlers), ") does not contain isr ", (void*)handle, "\n");
        KM_PANIC("IsrTable::release was given an entry that it was not responsible for.");
    }

    //
    // This const_cast is sound as we are certain it points to a member of the
    // array we own. And we are being executed in a non-const context, so this
    // is exactly equal to doing the work required to constitute the array element.
    //
    return const_cast<Entry*>(handle);
}

km::IsrCallback km::IsrTable::install(uint8_t isr, IsrCallback callback) {
    return mHandlers[isr].exchange(callback);
}

km::IsrContext km::IsrTable::invoke(km::IsrContext *context) {
    km::IsrCallback isr = mHandlers[uint8_t(context->vector)];
    return isr(context);
}

const km::IsrTable::Entry *km::IsrTable::allocate(IsrCallback callback) {
    //
    // We don't want to allow allocation in the architecturally reserved
    // range of idt entries. So take a subspan that only includes the
    // available area, which we will search for a free entry in.
    //
    std::span<Entry> available = std::span(mHandlers).subspan(isr::kExceptionCount);

    for (Entry& entry : available) {

        //
        // Find the first entry that is free by swapping with the default handler.
        // All free entries are denoted by containing `DefaultIsrHandler`.
        //
        IsrCallback expected = DefaultIsrHandler;
        if (entry.compare_exchange_strong(expected, callback)) {
            return &entry;
        }
    }

    return nullptr;
}

void km::IsrTable::release(const Entry *callback) {
    Entry *entry = find(callback);
    IsrCallback expected = callback->load();

    //
    // Only swap out the ISR handler with the default one if
    // it is still set to what the caller provided us. If it
    // has changed since then another thread has beaten us to
    // reusing it and we don't want to trample their work.
    //
    entry->compare_exchange_strong(expected, DefaultIsrHandler);
}

uint32_t km::IsrTable::index(const Entry *entry) const {
    if (std::begin(mHandlers) > entry || entry >= std::end(mHandlers)) {
        return UINT32_MAX;
    }

    return std::distance(mHandlers, entry);
}
