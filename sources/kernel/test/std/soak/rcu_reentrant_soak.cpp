#include "std/rcu/atomic.hpp"
#ifdef NDEBUG
#   undef NDEBUG
#endif

#include <random>

#include "std/rcu.hpp"
#include "reentrant.hpp"
#include "std/rcuptr.hpp"

#include <assert.h>

struct StringGenerator {
    std::mt19937 mt{ 0x1234 };
    std::uniform_int_distribution<> dist{ 0, CHAR_MAX };

    std::string operator()(size_t size = 32) {
        std::string str;
        for (size_t j = 0; j < size; j++) {
            str += dist(mt);
        }
        return str;
    }
};

int main() {
    static constexpr size_t kElementCount = 100000;
    static constexpr size_t kStringSize = 32;
    static constexpr size_t kThreadCount = 8;

    static std::atomic<bool> running = true;
    static std::atomic<size_t> inThread = 0;
    static std::atomic<size_t> inSignal = 0;
    static std::atomic<size_t> reclaims = 0;
    static std::atomic<size_t> inMainThread = 0;
    std::vector<ktest::IpSampleStorage> ipSamples;
    ipSamples.resize(kThreadCount);
    auto& ipSamplesMerged = ipSamples[0];

    {
        sm::RcuDomain domain;
        std::vector<sm::RcuAtomic<std::string>> data { kElementCount };
        std::vector<sm::RcuAtomic<std::string>> data2 { kElementCount };

        StringGenerator strings;

        for (size_t i = 0; i < kElementCount; i++) {
            data[i].reset(&domain, strings(kStringSize));
        }

        std::vector<size_t> indices;

        {
            std::mt19937 mt(0x1234);
            std::uniform_int_distribution<size_t> dist(0, kElementCount - 1);
            std::generate_n(std::back_inserter(indices), kElementCount, [&] {
                return dist(mt);
            });
        }

        std::atomic<size_t> currentIndex = 0;
        auto getNextIndex = [&] {
            size_t index = indices[currentIndex];
            currentIndex = (currentIndex + 1) % kElementCount;
            return index;
        };

        std::vector<pthread_t> threads;

        for (size_t i = 0; i < kThreadCount; i++) {
            std::vector<size_t> listIndices;
            std::mt19937 mt(0x1234);
            std::uniform_int_distribution<size_t> dist(0, kElementCount - 1);
            std::generate_n(std::back_inserter(listIndices), kElementCount, [&] {
                return dist(mt);
            });
            pthread_t thread = ktest::CreateReentrantThread([&domain, &data, &data2, listIndices] {
                size_t currentIndex = 0;
                auto getNextIndex = [&] {
                    size_t index = listIndices[currentIndex];
                    currentIndex = (currentIndex + 1) % kElementCount;
                    return index;
                };
                while (running) {
                    size_t i0 = getNextIndex();
                    size_t i1 = getNextIndex();
                    size_t i2 = getNextIndex();
                    sm::RcuGuard guard(domain);
                    if (sm::RcuShared object = data[i0].load()) {
                        data2[i1].store(object);
                        data[i2].reset();
                    }

                    if (sm::RcuShared object = data2[i2].load()) {
                        data[i1].store(object);
                    }

                    inThread += 1;
                }
            }, [&, i](siginfo_t *, mcontext_t *mc) {
                ipSamples[i].record(mc);
                size_t i0 = getNextIndex();
                size_t i1 = getNextIndex();
                sm::RcuGuard guard(domain);
                if (sm::RcuShared object = data[i0].load()) {
                    data2[i1].store(object);
                }

                inSignal += 1;
            });

            threads.push_back(thread);
        }

        auto now = std::chrono::high_resolution_clock::now();
        auto end = now + std::chrono::seconds(10);
        while (now < end) {
            reclaims += domain.synchronize();
            inMainThread += 1;

            // sm::RcuGuard guard(domain);

            // size_t i0 = getNextIndex();
            // data[i0].reset(&domain, new std::string(strings(kStringSize)));

            for (pthread_t thread : threads) {
                ktest::AlertReentrantThread(thread);
            }
            now = std::chrono::high_resolution_clock::now();
        }
        running = false;
        for (pthread_t thread : threads) {
            pthread_join(thread, nullptr);
        }
        for (size_t i = 1; i < ipSamples.size(); i++) {
            ipSamplesMerged.merge(ipSamples[i]);
        }
        ipSamplesMerged.dump(std::format("rcu_reentrant_soak_ip_samples/{}.txt", getpid()));

        data.clear();
        data2.clear();
        threads.clear();
    }

    fprintf(stderr, "inThread: %zu\n", inThread.load());
    fprintf(stderr, "inSignal: %zu\n", inSignal.load());
    fprintf(stderr, "reclaims: %zu\n", reclaims.load());
    fprintf(stderr, "inMainThread: %zu\n", inMainThread.load());
    fprintf(stderr, "ipSamples: %zu\n", ipSamplesMerged.size());
    assert(inThread.load() != 0);
    assert(inSignal.load() != 0);
    assert(reclaims.load() != 0);
    for (const auto& ipSamples : ipSamples) {
        assert(!ipSamples.isEmpty());
    }
}
