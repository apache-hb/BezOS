#include "process/schedule.hpp"
#include "kernel.hpp"

CPU_LOCAL
static constinit km::CpuLocal<sm::RcuSharedPtr<km::Thread>> tlsCurrentThread;

CPU_LOCAL
static constinit km::CpuLocal<uint8_t> tlsScheduleIdx;

extern "C" [[noreturn]] void KmResumeThread(km::IsrContext *context);

km::Scheduler::Scheduler()
    : mQueue()
{ }

void km::Scheduler::addWorkItem(sm::RcuSharedPtr<Thread> thread) {
    if (thread) {
        mQueue.enqueue(thread);
    }
}

sm::RcuSharedPtr<km::Thread> km::Scheduler::getWorkItem() noexcept {

    sm::RcuWeakPtr<Thread> thread;
    while (mQueue.try_dequeue(thread)) {
        if (sm::RcuSharedPtr<km::Thread> ptr = thread.lock()) {
            return ptr;
        }
    }

    return nullptr;
}

sm::RcuSharedPtr<km::Thread> km::GetCurrentThread() {
    return tlsCurrentThread.get();
}

sm::RcuSharedPtr<km::Process> km::GetCurrentProcess() {
    if (sm::RcuSharedPtr<km::Thread> thread = GetCurrentThread()) {
        return thread->process.lock();
    }

    return nullptr;
}

void km::SwitchThread(sm::RcuSharedPtr<km::Thread> next) {
    tlsCurrentThread = next;
    km::IsrContext *state = &next->state;

    //
    // If we're transitioning out of kernel space then we need to swapgs.
    //
    if ((state->cs & 0b11) != 0) {
        __swapgs();
    }

    KmResumeThread(state);
}

void km::ScheduleWork(LocalIsrTable *table, IApic *apic, sm::RcuSharedPtr<km::Thread> initial) {
    tlsCurrentThread = initial;

    const IsrEntry *scheduleInt = table->allocate([](km::IsrContext *ctx) noexcept -> km::IsrContext {
        IApic *apic = km::GetCpuLocalApic();
        apic->eoi();

        Scheduler *scheduler = km::GetScheduler();

        if (sm::RcuSharedPtr<km::Thread> next = scheduler->getWorkItem()) {
            if (sm::RcuSharedPtr<km::Thread> current = km::GetCurrentThread()) {
                KmDebugMessage("\bS");
                current->state = *ctx;
                scheduler->addWorkItem(current);
                tlsCurrentThread = next;
                return next->state;
            } else {
                KmDebugMessage("\bC");
                tlsCurrentThread = next;
                return next->state;
            }
        } else {
            KmDebugMessage("\bX");
            return *ctx;
        }
    });

    tlsScheduleIdx = table->index(scheduleInt);

    apic->setTimerDivisor(apic::TimerDivide::e64);
    apic->setInitialCount(0x10000);

    apic->cfgIvtTimer(apic::IvtConfig {
        .vector = tlsScheduleIdx.get(),
        .polarity = apic::Polarity::eActiveHigh,
        .trigger = apic::Trigger::eEdge,
        .enabled = true,
        .timer = apic::TimerMode::ePeriodic,
    });

    KmIdle();
}

void km::YieldCurrentThread() {
    IApic *apic = km::GetCpuLocalApic();
    apic->selfIpi(tlsScheduleIdx.get());
}
