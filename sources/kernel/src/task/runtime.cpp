#include "task/runtime.hpp"
#include "task/scheduler.hpp"
#include "task/scheduler_queue.hpp"
#include "system/thread.hpp"

#include "arch/xsave.hpp"
#include "isr/isr.hpp"
#include "thread.hpp"
#include "xsave.hpp"

extern "C" void __x86_64_idle() noexcept;

bool task::switchCurrentContext(Scheduler *scheduler, task::SchedulerQueue *queue, km::IsrContext *isrContext) noexcept {
    x64::XSave *xsave = nullptr;
    if (task::SchedulerEntry *current = queue->getCurrentTask()) {
        task::TaskState& currentState = current->getState();
        xsave = currentState.xsave;
        km::XSaveStoreState(xsave);
    }

    task::TaskState state {
        .registers = {
            .rax = isrContext->rax,
            .rbx = isrContext->rbx,
            .rcx = isrContext->rcx,
            .rdx = isrContext->rdx,
            .rdi = isrContext->rdi,
            .rsi = isrContext->rsi,
            .r8 = isrContext->r8,
            .r9 = isrContext->r9,
            .r10 = isrContext->r10,
            .r11 = isrContext->r11,
            .r12 = isrContext->r12,
            .r13 = isrContext->r13,
            .r14 = isrContext->r14,
            .r15 = isrContext->r15,
            .rbp = isrContext->rbp,
            .rsp = isrContext->rsp,
            .rip = isrContext->rip,
            .rflags = isrContext->rflags,
            .cs = isrContext->cs,
            .ss = isrContext->ss,
        },
        .xsave = xsave,
        .tlsBase = IA32_FS_BASE.load(),
    };

    if (scheduler->reschedule(queue, &state) == task::ScheduleResult::eIdle) {
        isrContext->rip = reinterpret_cast<uintptr_t>(__x86_64_idle);
        return false;
    }

    isrContext->rax = state.registers.rax;
    isrContext->rbx = state.registers.rbx;
    isrContext->rcx = state.registers.rcx;
    isrContext->rdx = state.registers.rdx;
    isrContext->rdi = state.registers.rdi;
    isrContext->rsi = state.registers.rsi;
    isrContext->r8 = state.registers.r8;
    isrContext->r9 = state.registers.r9;
    isrContext->r10 = state.registers.r10;
    isrContext->r11 = state.registers.r11;
    isrContext->r12 = state.registers.r12;
    isrContext->r13 = state.registers.r13;
    isrContext->r14 = state.registers.r14;
    isrContext->r15 = state.registers.r15;
    isrContext->rbp = state.registers.rbp;
    isrContext->rsp = state.registers.rsp;
    isrContext->rip = state.registers.rip;
    isrContext->rflags = state.registers.rflags;
    isrContext->cs = state.registers.cs;
    isrContext->ss = state.registers.ss;
    IA32_FS_BASE.store(state.tlsBase);
    km::XSaveLoadState(state.xsave);

    return true;
}
