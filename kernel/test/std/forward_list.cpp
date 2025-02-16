#include <gtest/gtest.h>

#include <latch>
#include <random>
#include <thread>

#include "std/forward_list.hpp"

template<typename T>
using Afl = sm::AtomicForwardList<T, std::allocator<T>>;

TEST(AtomicForwardListTest, Construct) {
    Afl<std::string> list;
}

TEST(AtomicForwardListTest, Push) {
    Afl<std::string> list;
    list.push("Hello world");
    list.push("Element 2");
    list.push("Lorum ipsum");

    ASSERT_EQ(list.pop("Otherwise"), "Lorum ipsum");
    ASSERT_EQ(list.pop("Otherwise"), "Element 2");
    ASSERT_EQ(list.pop("Otherwise"), "Hello world");
}

static std::string RandomString(int size, std::uniform_int_distribution<char> dist, std::mt19937& mt) {
    std::string result;
    result.reserve(size);

    for (int i = 0; i < size; i++) {
        result.push_back(dist(mt));
    }

    return result;
}

TEST(AtomicForwardListTest, Threads) {
    std::mt19937 mt(0x1234);
    static constexpr int kItemCount = 10000;

    std::set<std::string> inElements;

    std::mutex outMutex;
    std::set<std::string> outElements;

    static constexpr int kThreadCount = 4;

    // Construct random strings to put in the in elements set.
    std::uniform_int_distribution<char> dist('a', 'z');
    for (int i = 0; i < kItemCount; i++) {
        inElements.insert(RandomString(10, dist, mt));
    }

    std::vector<std::jthread> threads;
    std::latch latch(1);
    std::atomic<bool> done = false;
    Afl<std::string> list;

    for (int i = 0; i < kThreadCount; i++) {
        threads.emplace_back([&] {
            // Wait for the reader thread to start so we can ensure
            // both reading and writing are happening concurrently.
            latch.wait();

            for (const std::string& str : inElements) {
                list.push(str);
            }

            done = true;
        });
    }

    threads.emplace_back([&] {
        // Wait for the writer thread to start so we can ensure
        // both reading and writing are happening concurrently.
        latch.count_down();

        while (true) {
            auto head = list.pop("");

            if (head.empty()) {
                if (done) break;
                else continue;
            }

            // Add the element to the out set
            {
                std::lock_guard guard(outMutex);
                outElements.insert(head);
            }
        }
    });

    threads.clear();

    // If the input and output sets arent equal then we're not atomic.
    ASSERT_EQ(inElements, outElements);
}
