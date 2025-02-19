#include <gtest/gtest.h>

#include <latch>
#include <random>
#include <thread>

#include "std/rcu.hpp"
#include "std/rcuptr.hpp"

struct Tree {
    sm::RcuSharedPtr<Tree> left;
    sm::RcuSharedPtr<Tree> right;

    std::atomic<uint64_t> data;
};

static void AddElement(sm::RcuDomain *domain, sm::RcuSharedPtr<Tree> root, Tree *element) {
    sm::RcuSharedPtr<Tree> current = root;

    while (true) {
        if (element->data < current->data) {
            if (current->left == nullptr) {
                current->left = {domain, element};
                return;
            }

            current = current->left;
        } else {
            if (current->right == nullptr) {
                current->right = {domain, element};
                return;
            }

            current = current->right;
        }
    }
}

static void DeleteElement(sm::RcuSharedPtr<Tree> root, uint8_t data) {
    sm::RcuSharedPtr<Tree> parent = root;
    sm::RcuSharedPtr<Tree> current = root;
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
            return;
        }
    }

    // Element not found, delete the current element.
    if (left) {
        parent->left = nullptr;
    } else {
        parent->right = nullptr;
    }
}

static bool FindElement(sm::RcuSharedPtr<Tree> root, uint64_t data) {
    sm::RcuSharedPtr<Tree> current = root;

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

TEST(RcuPtrTest, Insert) {
    sm::RcuDomain domain;
    sm::RcuSharedPtr<Tree> root = {&domain, new Tree { nullptr, nullptr }};

    std::mt19937 mt(0x1234);
    std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

    // Build out some initial data.
    for (int i = 0; i < 100000; i++) {
        Tree *element = new Tree { nullptr, nullptr, dist(mt) };
        AddElement(&domain, root, element);
    }

    std::vector<std::jthread> threads;
    std::latch latch(2);
    std::atomic<bool> running = true;

    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&latch, &root] {
            std::mt19937 mt(0x9999);
            std::uniform_int_distribution<uint64_t> dist(0, UINT64_MAX);

            latch.wait();

            // traverse the tree and find a random element.
            // I don't care if it exists or not, just traverse the tree
            // so that we can see if the RCU domain is working correctly.
            for (int i = 0; i < 1000; i++) {
                FindElement(root, dist(mt));
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
                AddElement(&domain, root, element);
            } else {
                DeleteElement(root, dist(mt));
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

    // If we get here without crashing chances are the RCU domain is working.
    SUCCEED();
}

TEST(RcuPtrTest, Weak) {
    sm::RcuDomain domain;
    struct Pair {
        sm::RcuSharedPtr<std::string> strong;
        sm::RcuWeakPtr<std::string> weak;
    };

    std::vector<Pair> pairs;
    for (int i = 0; i < 100000; i++) {
        sm::RcuSharedPtr<std::string> strong = {&domain, new std::string("Hello, World!")};
        pairs.push_back({strong, strong});
    }

    std::vector<std::jthread> threads;
    std::atomic<bool> released = true;
    std::atomic<bool> error = false;
    std::atomic<bool> running = true;
    std::latch latch(2);

    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&] {
            latch.wait();

            // at least a few of these should fail.
            for (Pair& pair : pairs) {
                auto str = pair.weak.lock();
                if (str == nullptr) {
                    // The strong pointer has been deleted.
                    released = false;
                } else {
                    if (*str != "Hello, World!") {
                        error = true;
                    }
                }
            }
        });
    }

    threads.emplace_back([&] {
        std::mt19937 mt(0x1234);
        std::uniform_int_distribution<int> dist(0, pairs.size());

        latch.arrive_and_wait();
        // delete random elements.
        for (int i = 0; i < 1000; i++) {
            int index = dist(mt);
            pairs[index].strong = nullptr;
        }
    });

    std::jthread cleaner([&] {
        latch.arrive_and_wait();

        while (running) {
            domain.synchronize();
        }
    });

    threads.clear();
    running = false;
    cleaner.join();

    ASSERT_FALSE(released);
}

TEST(RcuPtrTest, Construct) {
    sm::RcuDomain domain;
    sm::RcuSharedPtr<std::string> ptr = {&domain, new std::string("Hello, World!")};
    ASSERT_EQ(*ptr, "Hello, World!");
    ptr = nullptr;
}

TEST(RcuPtrTest, ConstructWeak) {
    sm::RcuDomain domain;
    sm::RcuSharedPtr<std::string> ptr = {&domain, new std::string("Hello, World!")};
    ASSERT_EQ(*ptr, "Hello, World!");
    sm::RcuWeakPtr<std::string> weak = ptr;
    ptr = nullptr;
    ASSERT_EQ(weak.lock(), nullptr);
}

class IntrusiveData : public sm::RcuIntrusivePtr<IntrusiveData> {
    std::string mValue;

public:
    IntrusiveData(std::string value)
        : mValue(std::move(value))
    { }

    const std::string& value() const {
        return mValue;
    }
};

TEST(RcuPtrTest, Intrusive) {
    sm::RcuDomain domain;
    sm::RcuSharedPtr<IntrusiveData> ptr = sm::makeRcuShared<IntrusiveData>(&domain, "Hello, World!");
    ASSERT_EQ(ptr->value(), "Hello, World!");
    sm::RcuWeakPtr<IntrusiveData> weak = ptr;
    ptr = nullptr;
    ASSERT_EQ(weak.lock(), nullptr);
}
