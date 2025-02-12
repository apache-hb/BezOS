#include "process/schedule.hpp"

km::Scheduler::Scheduler()
    : mQueue()
{ }

void km::Scheduler::addWorkItem(km::ThreadId thread) {
    mQueue.enqueue(thread);
}

km::ThreadId km::Scheduler::getWorkItem() {
    km::ThreadId thread{};
    mQueue.try_dequeue(thread);
    return thread;
}
