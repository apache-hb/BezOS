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

sm::RcuSharedPtr<km::Thread> km::Scheduler::getWorkItem() {

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
    return GetCurrentThread()->process.lock();
}

static void SwitchThread(sm::RcuSharedPtr<km::Thread> next) {
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

void km::ScheduleWork(LocalIsrTable *table, IApic *apic) {
    tlsCurrentThread = nullptr;

    const IsrEntry *scheduleInt = table->allocate([](km::IsrContext *ctx) -> km::IsrContext {
        IApic *apic = km::GetCpuLocalApic();
        apic->eoi();

        Scheduler *scheduler = km::GetScheduler();
        sm::RcuSharedPtr<km::Thread> thread = scheduler->getWorkItem();

        if (thread) {
            if (sm::RcuSharedPtr<km::Thread> current = km::GetCurrentThread()) {
                current->state = *ctx;
                scheduler->addWorkItem(current);
            }

            SwitchThread(thread);
        }

        return *ctx;
    });
    tlsScheduleIdx = table->index(scheduleInt);

    apic->setTimerDivisor(apic::TimerDivide::e32);
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
