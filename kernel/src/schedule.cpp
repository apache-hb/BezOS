#include "schedule.hpp"

km::Scheduler::Scheduler()
    : mQueue()
{ }

void km::Scheduler::addWorkItem(km::ProcessThread *thread) {
    mQueue.enqueue(thread);
}

km::ProcessThread *km::Scheduler::getWorkItem() {
    km::ProcessThread *thread = nullptr;
    mQueue.try_dequeue(thread);
    return thread;
}

static constinit km::Scheduler *gScheduler = nullptr;

void km::InitScheduler(IsrAllocator& isrs) {
    gScheduler = new km::Scheduler();

    uint8_t isr = isrs.allocateIsr();

    InstallIsrHandler(isr, [](km::IsrContext *context) -> km::IsrContext {
        return *context;
    });
}
