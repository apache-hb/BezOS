#pragma once

#include <signal.h>
#include <pthread.h>
#include <utility>

namespace ktest {
    static constexpr int kReentrantSignal = SIGUSR1;

    template<typename Thread, typename Signal>
    pthread_t CreateReentrantThread(Thread&& run, Signal&& handler) {
        struct ThreadState {
            Thread thread;
            Signal signal;
        };

        ThreadState *state = new ThreadState {
            .thread = std::forward<Thread>(run),
            .signal = std::forward<Signal>(handler),
        };

        {
            sigset_t set;
            sigemptyset(&set);
            sigaddset(&set, kReentrantSignal);
            pthread_sigmask(SIG_BLOCK, &set, nullptr);
        }

        pthread_t thread;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&thread, &attr, [](void *arg) -> void* {
            static thread_local ThreadState *tlsState = reinterpret_cast<ThreadState*>(arg);

            struct sigaction sigusr1 {
                .sa_sigaction = [](int, [[maybe_unused]] siginfo_t *siginfo, [[maybe_unused]] void *ctx) {
                    ucontext_t *uc = reinterpret_cast<ucontext_t *>(ctx);
                    mcontext_t *mc = &uc->uc_mcontext;
                    tlsState->signal(siginfo, mc);
                },
                .sa_flags = SA_SIGINFO,
            };
            sigaction(kReentrantSignal, &sigusr1, nullptr);

            {
                sigset_t set;
                sigemptyset(&set);
                sigaddset(&set, kReentrantSignal);
                sigprocmask(SIG_UNBLOCK, &set, nullptr);
            }

            tlsState->thread();
            delete tlsState;
            return nullptr;
        }, state);
        pthread_attr_destroy(&attr);
        return thread;
    }

    inline void AlertReentrantThread(pthread_t thread) {
        pthread_sigqueue(thread, kReentrantSignal, {0});
    }
}
