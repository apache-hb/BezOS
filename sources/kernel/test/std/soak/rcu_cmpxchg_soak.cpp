#ifdef NDEBUG
#   undef NDEBUG
#endif

#include "std/rcu/atomic.hpp"
#include "std/rcu.hpp"

#include <thread>
#include <vector>
#include <random>

#include <cassert>

static std::atomic<size_t> sum = 0;
static std::atomic<size_t> syncronizes = 0;
static std::atomic<size_t> reclaimed = 0;
static std::atomic<size_t> swaps = 0;
static std::atomic<size_t> failedSwaps = 0;

static std::atomic<size_t> live = 0;

struct InnerObject {
    unsigned value;

    UTIL_NOCOPY(InnerObject);
    UTIL_NOMOVE(InnerObject);

    InnerObject(unsigned d)
        : value(d)
    {
        live += 1;
    }

    ~InnerObject() {
        live -= 1;
    }
};

int main() {
    static constexpr size_t kThreadCount = 8;
    static constexpr size_t kObjectCount = 1024 * 1024;

    {
        sm::RcuDomain domain;

        std::vector<std::jthread> threads;
        std::atomic<bool> running = true;

        std::mt19937 mt{0x1234};
        std::uniform_int_distribution<size_t> dist(0, kObjectCount - 1);
        std::unique_ptr<sm::RcuAtomic<InnerObject>[]> objects(new sm::RcuAtomic<InnerObject>[kObjectCount]);

        for (size_t i = 0; i < kObjectCount; i++) {
            objects[i].reset(&domain, dist(mt));
        }

        for (size_t i = 0; i < kThreadCount; i++) {
            threads.emplace_back([&, i] {
                std::mt19937 mt{i};
                std::uniform_int_distribution<size_t> dist(0, kObjectCount - 1);
                while (running) {
                    size_t index0 = dist(mt);
                    size_t index1 = dist(mt);
                    size_t op = dist(mt) % 2;

                    {
                        sm::RcuGuard guard(domain);
                        sm::RcuShared<InnerObject> object0 = objects[index0].load();
                        sm::RcuShared<InnerObject> object1 = objects[index1].load();
                        if (op == 0) {
                            if (objects[index0].compare_exchange_weak(object0, object0)) {
                                sum += object0.get()->value;
                                swaps += 1;
                            } else {
                                failedSwaps += 1;
                            }
                        } else if (op == 1) {
                            if (objects[index0].compare_exchange_weak(object1, object0)) {
                                swaps += 1;
                            } else {
                                failedSwaps += 1;
                            }
                        }
                    }
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

    fprintf(stderr, "syncronizes: %zu, reclaimed: %zu, live: %zu, swaps: %zu, failedSwaps: %zu\n",
            syncronizes.load(), reclaimed.load(), live.load(), swaps.load(), failedSwaps.load());

    assert(syncronizes > 0);
    assert(reclaimed > 0);
    assert(sum > 0);
    assert(live == 0);
    assert(swaps > 0);
    assert(failedSwaps > 0);
}
