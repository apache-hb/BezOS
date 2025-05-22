#ifdef NDEBUG
#   undef NDEBUG
#endif

#include "std/rcu.hpp"
#include <thread>
#include <vector>

#include <cassert>

int main() {
    static constexpr size_t kThreadCount = 8;

    static std::atomic<size_t> mallocs = 0;
    static std::atomic<size_t> frees = 0;
    static std::atomic<size_t> syncronizes = 0;
    static std::atomic<size_t> reclaimed = 0;

    {
        sm::RcuDomain domain;

        std::vector<std::jthread> threads;
        std::atomic<bool> running = true;

        for (size_t i = 0; i < kThreadCount; i++) {
            threads.emplace_back([&] {
                while (running) {
                    void *data = malloc(256);
                    mallocs += 1;
                    OsStatus status = domain.call(data, [](void *data) {
                        free(data);
                        frees += 1;
                    });
                    assert(status == OsStatusSuccess);
                }
            });
        }

        threads.emplace_back([&] {
            while (running) {
                reclaimed += domain.synchronize();
                syncronizes += 1;
            }
        });

        std::this_thread::sleep_for(std::chrono::seconds(10));
        running = false;
        for (auto &thread : threads) {
            thread.join();
        }
    }

    fprintf(stderr, "mallocs: %zu, frees: %zu, syncronizes: %zu, reclaimed: %zu\n",
           mallocs.load(), frees.load(), syncronizes.load(), reclaimed.load());

    assert(mallocs > 0);
    assert(frees > 0);
    assert(mallocs == frees);
    // assert(syncronizes > 0);
    assert(reclaimed > 0);
}
