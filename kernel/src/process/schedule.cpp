#include "process/schedule.hpp"
#include "kernel.hpp"

CPU_LOCAL
static constinit km::CpuLocal<km::Thread*> tlsCurrentThread;

static constinit stdx::SpinLock gSchedulerLock;

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

static void SwitchThread(km::Thread *next) {
    tlsCurrentThread = next;
    // RestoreThread(next);
}

void km::ScheduleWork(IsrTable *table, IApic *apic) {
    const IsrTable::Entry *scheduleInt = table->allocate([](km::IsrContext *ctx) -> km::IsrContext {
        Scheduler *scheduler = km::GetScheduler();
        IApic *apic = km::GetCpuLocalApic();
        km::Thread *thread = scheduler->getWorkItem();
        if (thread) {
            stdx::LockGuard guard(gSchedulerLock);

            km::Thread *current = km::GetCurrentThread();
            current->state = *ctx;
            scheduler->addWorkItem(current);
            SwitchThread(thread);
        }

        return *ctx;
    });
}
