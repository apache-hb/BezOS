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
