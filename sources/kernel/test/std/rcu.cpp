#include <gtest/gtest.h>

#include <latch>
#include <random>
#include <thread>

#include "std/rcu.hpp"
#include "reentrant.hpp"
#include "std/rcuptr.hpp"

TEST(RcuTest, Construct) {
    sm::RcuDomain domain;
}

TEST(RcuTest, SynchronizeEmpty) {
    sm::RcuDomain domain;
    domain.synchronize();
}

struct Tree : public sm::RcuObject {
    std::atomic<Tree*> left;
    std::atomic<Tree*> right;

    std::atomic<uint64_t> data;

    Tree(Tree *l, Tree *r, uint64_t d = 0)
        : left(l), right(r), data(d)
    { }
};

static void AddElement(sm::RcuDomain& domain, Tree *root, Tree *element) {
    sm::RcuGuard guard(domain);
    Tree *current = root;

    while (true) {
        if (element->data < current->data) {
            Tree *expected = nullptr;
            if (current->left.compare_exchange_strong(expected, element)) {
                return;
            }

            current = current->left;
        } else {
            Tree *expected = nullptr;
            if (current->right.compare_exchange_strong(expected, element)) {
                return;
            }

            current = current->right;
        }
    }
}

static void DeleteElement(sm::RcuDomain& domain, Tree *root, uint8_t data) {
    sm::RcuGuard guard(domain);

    Tree *parent = root;
    Tree *current = root;
    bool left = false;

    while (current != nullptr) {
        parent = current;

        left = data < current->data;
        if (left) {
            current = current->left;
        } else {
            current = current->right;
        }

        if (current && current->data == data) {
            if (left) {
                parent->left = nullptr;
            } else {
                parent->right = nullptr;
            }

            domain.retire(current);
            return;
        }
    }

    // Element not found, delete the current element.
    if (left) {
        parent->left = nullptr;
    } else {
        parent->right = nullptr;
    }

    if (current == nullptr) {
        return;
    }

    domain.retire(current);
}

static bool FindElement(sm::RcuDomain& domain, Tree *root, uint64_t data) {
    sm::RcuGuard guard(domain);
    Tree *current = root;

    while (current != nullptr) {
        if (current->data == data) {
            return true;
        }

        if (data < current->data) {
            current = current->left;
        } else {
            current = current->right;
        }
    }

    return false;
}

static void DestroyTree(sm::RcuDomain& domain, Tree *root) {
    if (root == nullptr) {
        return;
    }

    sm::RcuGuard guard(domain);

    DestroyTree(domain, root->left);
    DestroyTree(domain, root->right);

    domain.retire(root);
}

TEST(RcuTest, Insert) {
    sm::RcuDomain domain;
    Tree *root = new Tree { nullptr, nullptr };

    std::mt19937 mt(0x1234);
    std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

    // Build out some initial data.
    for (int i = 0; i < 100000; i++) {
        Tree *element = new Tree { nullptr, nullptr, dist(mt) };
        AddElement(domain, root, element);
    }

    std::vector<std::jthread> threads;
    std::latch latch(2);
    std::atomic<bool> running = true;

    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&latch, &domain, &root] {
            std::mt19937 mt(0x9999);
            std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

            latch.wait();

            // traverse the tree and find a random element.
            // I don't care if it exists or not, just traverse the tree
            // so that we can see if the RCU domain is working correctly.
            for (int i = 0; i < 1000; i++) {
                FindElement(domain, root, dist(mt));
            }
        });
    }

    threads.emplace_back([&latch, &domain, &root] {
        std::mt19937 mt(0x9999);
        std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

        latch.arrive_and_wait();

        for (int i = 0; i < 1000; i++) {
            if (i % 2) {
                Tree *element = new Tree { nullptr, nullptr, dist(mt) };
                AddElement(domain, root, element);
            } else {
                DeleteElement(domain, root, dist(mt));
            }
        }
    });

    std::jthread cleaner([&latch, &domain, &running] {
        latch.arrive_and_wait();

        while (running) {
            domain.synchronize();
        }
    });

    threads.clear();
    running = false;
    cleaner.join();

    // cleanup the tree.
    DestroyTree(domain, root);

    // If we get here without crashing chances are the RCU domain is working.
    SUCCEED();
}

TEST(RcuTest, ReentrantReclaimation) {
    static constexpr size_t kElementCount = 100000;
    static constexpr size_t kStringSize = 32;

    sm::RcuDomain domain;
    std::vector<sm::RcuSharedPtr<std::string>> data;
    std::vector<sm::RcuSharedPtr<std::string>> data2;
    data.resize(kElementCount);
    data2.resize(kElementCount);

    std::mt19937 mt(0x1234);
    std::uniform_int_distribution<> dist(0, CHAR_MAX);
    for (size_t i = 0; i < kElementCount; i++) {
        std::string str;
        for (size_t j = 0; j < kStringSize; j++) {
            str += dist(mt);
        }

        data[i] = sm::rcuMakeShared<std::string>(&domain, str);
    }

    std::atomic<bool> running = true;
    std::atomic<size_t> inThread = 0;
    std::atomic<size_t> inSignal = 0;
    std::atomic<size_t> reclaims = 0;

    std::vector<size_t> indices;
    std::generate_n(std::back_inserter(indices), kElementCount, [&] {
        return dist(mt);
    });
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
    }, [&](siginfo_t *, mcontext_t *) {
        size_t i0 = indices[getNextIndex()];
        size_t i1 = indices[getNextIndex()];
        data2[i1] = data[i0];

        inSignal += 1;
    });

    auto now = std::chrono::high_resolution_clock::now();
    auto end = now + std::chrono::milliseconds(100);
    while (now < end) {
        reclaims += domain.synchronize();
        ktest::AlertReentrantThread(thread);
        now = std::chrono::high_resolution_clock::now();
    }
    running = false;
    pthread_join(thread, nullptr);

    ASSERT_NE(inThread.load(), 0);
    ASSERT_NE(inSignal.load(), 0);
}
