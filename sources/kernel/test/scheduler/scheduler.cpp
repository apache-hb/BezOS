#include <gtest/gtest.h>

#include <setjmp.h>

#include "process/schedule.hpp"

TEST(SchedulerTest, Construct) {
    km::Scheduler scheduler;
}

struct ScheduleImpl {
    moodycamel::ConcurrentQueue<jmp_buf> queue;
};

TEST(SchedulerTest, ContextSwitch) {
    struct sigaction action {
        .sa_sigaction = [](int, siginfo_t *info, void *) {
            ScheduleImpl *impl = (ScheduleImpl*)info->si_value.sival_ptr;
            jmp_buf buf{};
            if (setjmp(buf) == 0) {
                impl->queue.enqueue(buf);
            }

            if (!impl->queue.try_dequeue(buf)) {
                return;
            }

            longjmp(buf, 1);
        },
        .sa_flags = SA_SIGINFO,
    };

    sigaction(SIGUSR1, &action, nullptr);

    ScheduleImpl impl;

    std::vector<pthread_t> threads;
    for (int i = 0; i < 10; i++) {
        pthread_t thread;
        pthread_create(&thread, nullptr, [](void *arg) -> void* {
            ScheduleImpl *impl = (ScheduleImpl*)arg;
            sigval value { .sival_ptr = impl };
            sigqueue(getpid(), SIGUSR1, value);

            return nullptr;
        }, &impl);

        threads.push_back(thread);
    }
}
