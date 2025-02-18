#include "process/schedule.hpp"
#include "kernel.hpp"

CPU_LOCAL
static constinit km::CpuLocal<km::Thread*> tlsCurrentThread;

CPU_LOCAL
static constinit km::CpuLocal<uint8_t> tlsScheduleIdx;

extern "C" [[noreturn]] void KmResumeThread(km::IsrContext *context);

km::Scheduler::Scheduler()
    : mQueue()
{ }

void km::Scheduler::addWorkItem(km::Thread *thread) {
    mQueue.enqueue(thread);
}

km::Thread *km::Scheduler::getWorkItem() {
    km::Thread *thread = nullptr;
    mQueue.try_dequeue(thread);
    return thread;
}

km::Thread *km::GetCurrentThread() {
    return tlsCurrentThread.get();
}

km::Process *km::GetCurrentProcess() {
    return GetCurrentThread()->process;
}

static void SwitchThread(km::Thread *next) {
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

void km::ScheduleWork(IsrTable *table, IApic *apic) {
    tlsCurrentThread = nullptr;

    const IsrTable::Entry *scheduleInt = table->allocate([](km::IsrContext *ctx) -> km::IsrContext {
        IApic *apic = km::GetCpuLocalApic();
        apic->eoi();

        Scheduler *scheduler = km::GetScheduler();
        km::Thread *thread = scheduler->getWorkItem();

        if (thread) {
            if (km::Thread *current = km::GetCurrentThread()) {
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
