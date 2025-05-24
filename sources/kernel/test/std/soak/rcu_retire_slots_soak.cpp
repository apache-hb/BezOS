#ifdef NDEBUG
#   undef NDEBUG
#endif

#include "std/detail/counted.hpp"
#include "std/rcu.hpp"

#include <thread>
#include <vector>
#include <random>

#include <cassert>

static std::atomic<size_t> sum = 0;
static std::atomic<size_t> syncronizes = 0;
static std::atomic<size_t> reclaimed = 0;

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

using Object = sm::detail::CountedObject<InnerObject>;

struct AtomicSharedPtr {
    std::atomic<Object*> mControl;

    AtomicSharedPtr(sm::RcuDomain *domain)
        : AtomicSharedPtr(domain, 0)
    { }

    AtomicSharedPtr(sm::RcuDomain *domain, unsigned d)
        : mControl(new Object(domain, d))
    { }

    AtomicSharedPtr(AtomicSharedPtr&& other)
        : mControl(other.mControl.load())
    {
        other.mControl.store(nullptr);
    }

    Object *load() {
        Object *control = mControl.load();

        if (control != nullptr && control->retainStrong(1)) {
            return control;
        }

        return nullptr;
    }

    void store(sm::RcuGuard& guard, Object *control) {
        Object *old = mControl.exchange(control);
        if (old != control) {
            if (control != nullptr) {
                control->retainStrong(1);
            }

            if (old != nullptr) {
                old->deferReleaseStrong(guard, 1);
            }
        }
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
        struct Storage {
            alignas(alignof(AtomicSharedPtr)) char data[sizeof(AtomicSharedPtr)];
        };
        std::unique_ptr<Storage[]> storage(new Storage[kObjectCount]);
        AtomicSharedPtr *objects = reinterpret_cast<AtomicSharedPtr*>(storage.get());
        for (size_t i = 0; i < kObjectCount; i++) {
            new (&storage[i]) AtomicSharedPtr(&domain);
            objects[i].mControl.load()->get().value = dist(mt);
        }

        for (size_t i = 0; i < kThreadCount; i++) {
            threads.emplace_back([&, i] {
                std::mt19937 mt{i};
                std::uniform_int_distribution<size_t> dist(0, kObjectCount - 1);
                while (running) {
                    size_t index0 = dist(mt);
                    size_t index1 = dist(mt);
                    size_t op = dist(mt) % 3;
                    Object *object = nullptr;

                    {
                        sm::RcuGuard guard(domain);
                        object = objects[index0].load();
                        if (op == 0) {
                            if (object) {
                                sum += object->get().value;
                            }
                        } else if (op == 1) {
                            if (object) {
                                objects[index1].store(guard, object);
                            }
                        }
                    }

                    if (object) {
                        sm::RcuGuard guard(domain);
                        object->deferReleaseStrong(guard, 1);
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

        sm::RcuGuard guard(domain);
        for (size_t i = 0; i < kObjectCount; i++) {
            if (Object *object = objects[i].mControl.load()) {
                object->deferReleaseStrong(guard, 1);
            }
        }
    }

    fprintf(stderr, "syncronizes: %zu, reclaimed: %zu, live: %zu\n",
           syncronizes.load(), reclaimed.load(), live.load());

    assert(syncronizes > 0);
    assert(reclaimed > 0);
    assert(sum > 0);
    assert(live == 0);
}
