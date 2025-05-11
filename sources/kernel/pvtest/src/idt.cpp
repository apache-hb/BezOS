#include "pvtest/idt.hpp"
#include "arch/isr.hpp"
#include "gdt.hpp"
#include "pvtest/cpu.hpp"
#include "pvtest/pvtest.hpp"

void pv::CpuIdt::ltr(uint16_t selector) {
    mTssSelector = selector;
}

void pv::CpuIdt::lgdt(GDTR ptr) {
    mGdtPointer = ptr;
}

void pv::CpuIdt::lidt(IDTR ptr) {
    mIdtPointer = ptr;
}

x64::TaskStateSegment *pv::CpuIdt::getTaskStateSegment() {
    uint16_t limit = mGdtPointer.limit;
    uint16_t offset = (mTssSelector * sizeof(x64::GdtEntry));
    if ((offset + sizeof(x64::TssEntry)) > (limit + 1)) {
        SignalAssert("Invalid TSS selector %u\n", mTssSelector);
    }

    const x64::TssEntry *entry = (x64::TssEntry*)(mGdtPointer.base + offset);
    if (!entry->isPresent()) {
        SignalAssert("TSS entry %u not present\n", mTssSelector);
    }

    return (x64::TaskStateSegment*)entry->address();
}

void pv::CpuIdt::invokeExceptionHandler(mcontext_t *mcontext, uintptr_t rip, uintptr_t rsp) {
    uint64_t *stack = (uint64_t*)rsp;
    stack[-1] = GDT_64BIT_DATA * 0x8; // ss
    stack[-2] = mcontext->gregs[REG_RSP]; // old rsp
    stack[-3] = 0x202; // rflags
    stack[-4] = GDT_64BIT_CODE * 0x8; // cs
    stack[-5] = mcontext->gregs[REG_RIP]; // rip

    gregset_t gregs;
    memcpy(gregs, mcontext->gregs, sizeof(gregs));
    gregs[REG_RIP] = rip;
    gregs[REG_RSP] = (greg_t)(stack - 5);

    PvTestContextSwitch(&gregs);
}

void pv::CpuIdt::doublefault(mcontext_t *mcontext) {
    SignalAssert("Double fault at %p\n", (void*)mcontext->gregs[REG_RIP]);
}

void pv::CpuIdt::raise(mcontext_t *mcontext, uint8_t vector) {
    uint16_t limit = mIdtPointer.limit;
    uint16_t offset = (vector * sizeof(x64::IdtEntry));
    if ((offset + sizeof(x64::IdtEntry)) > (limit + 1)) {
        doublefault(mcontext);
        return;
    }

    uint64_t base = mIdtPointer.base + offset;
    const x64::IdtEntry *entry = (x64::IdtEntry*)base;

    if (!entry->isPresent() || !entry->isInterruptGate()) {
        doublefault(mcontext);
        return;
    }

    uintptr_t rip = entry->address();
    x64::TaskStateSegment *tss = getTaskStateSegment();
    uintptr_t rsp = tss->rsp0;

    invokeExceptionHandler(mcontext, rip, rsp);
}
