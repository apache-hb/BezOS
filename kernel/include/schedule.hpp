#pragma once

#include "crt.hpp"
#include "process.hpp"

#define MOODYCAMEL_MALLOC ::malloc
#define MOODYCAMEL_FREE ::free

#include <concurrentqueue.h>

namespace km {
    class Scheduler {
        moodycamel::ConcurrentQueue<km::ProcessThread*> mQueue;

    public:
        Scheduler();

        void addWorkItem(km::ProcessThread *thread);
        km::ProcessThread *getWorkItem();
    };

    void InitScheduler();
}
