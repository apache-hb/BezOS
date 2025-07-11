#pragma once

#include <cerrno>
#include <cstring>
#include <format>
#include <pthread.h>
#include <stdexcept>
#include <utility>

#include "common/util/util.hpp"

namespace ktest {
    class PThread {
        pthread_t mThread;

    public:
        pthread_t getHandle() const {
            return mThread;
        }

        PThread() : mThread(0) { }

        UTIL_NOCOPY(PThread);

        PThread(PThread&& other) noexcept : mThread(other.mThread) {
            other.mThread = 0;
        }

        PThread& operator=(PThread&& other) noexcept {
            if (this != &other) {
                std::swap(mThread, other.mThread);
            }
            return *this;
        }

        template<typename F>
        explicit PThread(int sched, F&& func) {
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            if (pthread_attr_setschedpolicy(&attr, SCHED_FIFO)) {
                throw std::runtime_error(std::format("Failed to set thread scheduling policy {}", strerror(errno)));
            }

            sched_param param;
            param.sched_priority = sched;
            if (pthread_attr_setschedparam(&attr, &param)) {
                throw std::runtime_error(std::format("Failed to set thread priority {}", strerror(errno)));
            }

            if (pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED)) {
                throw std::runtime_error(std::format("Failed to set thread inheritsched {}", strerror(errno)));
            }

            int status = pthread_create(&mThread, &attr, [](void *arg) -> void* {
                F *f = static_cast<F*>(arg);
                (*f)();
                delete f;
                return nullptr;
            }, new F(std::forward<F>(func)));

            if (status != 0) {
                throw std::runtime_error(std::format("Failed to create thread: {}", strerror(status)));
            }

            pthread_attr_destroy(&attr);
        }

        void join() {
            if (mThread) {
                pthread_join(mThread, nullptr);
            }
        }

        ~PThread() {
            if (mThread) {
                pthread_cancel(mThread);
                pthread_join(mThread, nullptr);
            }

            mThread = 0;
        }
    };
}
