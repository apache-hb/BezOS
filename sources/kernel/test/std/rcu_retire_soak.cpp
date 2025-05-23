#include <random>
#ifdef NDEBUG
#   undef NDEBUG
#endif

#include "std/rcu.hpp"
#include <thread>
#include <vector>

#include <cassert>

struct ObjectWithData : public sm::RcuObject {
    unsigned data;

    ObjectWithData(unsigned d)
        : data(d)
    { }
};

int main() {
    static constexpr size_t kThreadCount = 8;
    static constexpr size_t kObjectCount = 1024 * 1024;

    static std::atomic<size_t> sum = 0;
    static std::atomic<size_t> mallocs = 0;
    static std::atomic<size_t> frees = 0;
    static std::atomic<size_t> syncronizes = 0;
    static std::atomic<size_t> reclaimed = 0;

    {
        sm::RcuDomain domain;

        std::vector<std::jthread> threads;
        std::atomic<bool> running = true;

        std::mt19937 mt{0x1234};
        std::uniform_int_distribution<size_t> dist(0, kObjectCount - 1);
        std::vector<std::atomic<ObjectWithData*>> objects { kObjectCount };
        for (size_t i = 0; i < kObjectCount; i++) {
            mallocs += 1;
            objects[i].store(new ObjectWithData(dist(mt)));
        }

        for (size_t i = 0; i < kThreadCount; i++) {
            threads.emplace_back([&, i] {
                std::mt19937 mt{i};
                std::uniform_int_distribution<size_t> dist(0, kObjectCount - 1);
                while (running) {
                    sm::RcuGuard guard(domain);
                    size_t index = dist(mt);
                    size_t op = dist(mt) % 2;
                    if (op == 0) {
                        ObjectWithData *object = objects[index].load();
                        if (object != nullptr) {
                            sum += object->data;
                        }
                    } else {
                        mallocs += 1;
                        ObjectWithData *object = objects[index].exchange(new ObjectWithData(dist(mt)));
                        if (object != nullptr) {
                            sm::RcuGuard inner(domain);
                            assert(inner == guard);
                            guard.retire(object);
                            frees += 1;
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

        for (size_t i = 0; i < kObjectCount; i++) {
            ObjectWithData *object = objects[i].load();
            if (object != nullptr) {
                delete object;
                frees += 1;
            }
        }
    }

    fprintf(stderr, "mallocs: %zu, frees: %zu, syncronizes: %zu, reclaimed: %zu\n",
           mallocs.load(), frees.load(), syncronizes.load(), reclaimed.load());

    assert(mallocs > 0);
    assert(frees > 0);
    assert(mallocs == frees);
    assert(syncronizes > 0);
    assert(reclaimed > 0);
    assert(sum > 0);
}
