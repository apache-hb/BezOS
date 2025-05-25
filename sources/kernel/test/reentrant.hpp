#pragma once

#include "allocator/tlsf.hpp"
#include "util/memory.hpp"

#include <dlfcn.h>
#include <format>
#include <signal.h>
#include <pthread.h>

#include <filesystem>
#include <map>
#include <memory>
#include <utility>
#include <fstream>

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

    class IpSampleStorage {
        std::unique_ptr<std::byte[]> mStorage;
        mem::TlsfAllocator mAllocator;

        using Allocator = mem::AllocatorPointer<std::pair<const uintptr_t, uintptr_t>>;
        std::map<uintptr_t, uintptr_t, std::less<uintptr_t>, Allocator> mIpSamples;
    public:
        IpSampleStorage(size_t size = sm::megabytes(1).bytes())
            : mStorage(new std::byte[size])
            , mAllocator(mStorage.get(), size)
            , mIpSamples(Allocator(&mAllocator))
        { }

        void record(const mcontext_t *mc) {
            uintptr_t ip = mc->gregs[REG_RIP];
            mIpSamples[ip] += 1;
        }

        bool isEmpty() const {
            return mIpSamples.empty();
        }

        void merge(const IpSampleStorage& other) {
            for (const auto& [ip, count] : other.mIpSamples) {
                mIpSamples[ip] += count;
            }
        }

        size_t size() const {
            return mIpSamples.size();
        }

        void dump(const std::filesystem::path& path) const {
            std::filesystem::create_directories(path.parent_path());
            std::ofstream file(path);
            for (const auto& [ip, count] : mIpSamples) {
                Dl_info info;
                dladdr((void*)ip, &info);
                size_t baseAddress = (size_t)info.dli_fbase;
                size_t offset = (size_t)ip - baseAddress;
                file << std::format("0x{:x} {}\n", offset, count);
            }
        }
    };
}
