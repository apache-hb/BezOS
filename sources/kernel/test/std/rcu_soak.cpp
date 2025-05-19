#include <gtest/gtest.h>

#include <random>

#include "std/rcu.hpp"
#include "reentrant.hpp"
#include "std/rcuptr.hpp"

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

TEST(RcuTest, ReentrantReclaimation) {
    static constexpr size_t kElementCount = 100000;
    static constexpr size_t kStringSize = 32;

    sm::RcuDomain domain;
    std::vector<sm::RcuSharedPtr<std::string>> data;
    std::vector<sm::RcuSharedPtr<std::string>> data2;
    data.resize(kElementCount);
    data2.resize(kElementCount);

    StringGenerator strings;

    for (size_t i = 0; i < kElementCount; i++) {
        data[i] = sm::rcuMakeShared<std::string>(&domain, strings(kStringSize));
    }

    std::atomic<bool> running = true;
    std::atomic<size_t> inThread = 0;
    std::atomic<size_t> inSignal = 0;
    std::atomic<size_t> reclaims = 0;

    ktest::IpSampleStorage ipSamples;

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

    pthread_t thread = ktest::CreateReentrantThread([&] {
        std::mt19937 mt(0x1234);
        std::uniform_int_distribution<size_t> dist(0, kElementCount - 1);
        while (running) {
            size_t i0 = dist(mt);
            size_t i1 = dist(mt);
            size_t i2 = dist(mt);
            data2[i1] = data[i0];
            data[i2].reset();

            inThread += 1;
        }
    }, [&](siginfo_t *, mcontext_t *mc) {
        ipSamples.record(mc);
        size_t i0 = indices[getNextIndex()];
        size_t i1 = indices[getNextIndex()];
        data2[i1] = data[i0];

        inSignal += 1;
    });

    auto now = std::chrono::high_resolution_clock::now();
    auto end = now + std::chrono::seconds(60);
    while (now < end) {
        reclaims += domain.synchronize();

        size_t i0 = indices[getNextIndex()];
        data[i0] = sm::rcuMakeShared<std::string>(&domain, strings(kStringSize));

        ktest::AlertReentrantThread(thread);
        now = std::chrono::high_resolution_clock::now();
    }
    running = false;
    pthread_join(thread, nullptr);
    ipSamples.dump("rcu_ip_samples.txt");

    ASSERT_NE(inThread.load(), 0);
    ASSERT_NE(inSignal.load(), 0);
    ASSERT_FALSE(ipSamples.isEmpty());
}
