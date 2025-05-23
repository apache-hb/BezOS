#include <gtest/gtest.h>

#include <latch>
#include <random>
#include <thread>

#include "std/rcu.hpp"
#include "std/rcuptr.hpp"

#include "common/compiler/compiler.hpp"

std::string RandomString(int length, std::mt19937& mt) {
    std::string str;
    str.reserve(length);

    std::uniform_int_distribution<int> dist('A', 'Z');

    for (int i = 0; i < length; i++) {
        str.push_back(dist(mt));
    }

    return str;
}

struct Node : public sm::RcuIntrusivePtr<Node> {
    std::string value;
    sm::RcuSharedPtr<Node> next;
};

struct Queue {
    sm::RcuSharedPtr<Node> head;
};

#if 0
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
#endif

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
    sm::RcuGuard guard(domain);
    sm::RcuSharedPtr<IntrusiveData> ptr = sm::rcuMakeShared<IntrusiveData>(&domain, "Hello, World!");
    ASSERT_EQ(ptr->value(), "Hello, World!");
    sm::RcuWeakPtr<IntrusiveData> weak = ptr;
    ptr = nullptr;
    ASSERT_EQ(weak.lock(), nullptr);
}

TEST(RcuPtrTest, Construct) {
    sm::RcuDomain domain;
    sm::RcuGuard guard(domain);
    sm::RcuSharedPtr<std::string> ptr = {&domain, new std::string("Hello, World!")};
    ASSERT_EQ(*ptr, "Hello, World!");
}

TEST(RcuPtrTest, AssignNull) {
    sm::RcuDomain domain;
    sm::RcuSharedPtr<std::string> ptr = {&domain, new std::string("Hello, World!")};
    ASSERT_EQ(domain.synchronize(), 0);
    ASSERT_EQ(*ptr, "Hello, World!");
    ptr = nullptr;
    ASSERT_EQ(domain.synchronize(), 2);
}

TEST(RcuPtrTest, AssignEmpty) {
    sm::RcuDomain domain;
    sm::RcuSharedPtr<std::string> ptr = {&domain, new std::string("Hello, World!")};
    sm::RcuSharedPtr<std::string> empty;
    ASSERT_EQ(domain.synchronize(), 0);
    ASSERT_EQ(*ptr, "Hello, World!");
    ptr = empty;
    ASSERT_EQ(domain.synchronize(), 2);
}

TEST(RcuPtrTest, Assign) {
    sm::RcuDomain domain;
    sm::RcuSharedPtr<std::string> ptr = {&domain, new std::string("Hello, World!")};
    ASSERT_EQ(*ptr, "Hello, World!");
    ptr = sm::rcuMakeShared<std::string>(&domain, "Goodbye, World!");
    ASSERT_EQ(*ptr, "Goodbye, World!");
}

TEST(RcuPtrTest, CopyAssign) {
    sm::RcuDomain domain;
    sm::RcuSharedPtr<std::string> ptr = {&domain, new std::string("Hello, World!")};
    ASSERT_EQ(*ptr, "Hello, World!");
    sm::RcuSharedPtr<std::string> other = sm::rcuMakeShared<std::string>(&domain, "Goodbye, World!");
    ASSERT_EQ(domain.synchronize(), 0);
    ptr = other;
    ASSERT_EQ(domain.synchronize(), 2);
    ASSERT_EQ(*ptr, "Goodbye, World!");
}

TEST(RcuPtrTest, CopyAssignSelf) {
    sm::RcuDomain domain;
    sm::RcuSharedPtr<std::string> ptr = {&domain, new std::string("Hello, World!")};
    ASSERT_EQ(*ptr, "Hello, World!");
    ASSERT_EQ(domain.synchronize(), 0);
    CLANG_DIAGNOSTIC_PUSH();
    CLANG_DIAGNOSTIC_IGNORE("-Wself-assign-overloaded");
    ptr = ptr;
    CLANG_DIAGNOSTIC_POP();
    ASSERT_EQ(domain.synchronize(), 0);
    ASSERT_EQ(*ptr, "Hello, World!");
}

TEST(RcuPtrTest, Reset) {
    sm::RcuDomain domain;
    sm::RcuSharedPtr<std::string> ptr = {&domain, new std::string("Hello, World!")};
    ASSERT_EQ(*ptr, "Hello, World!");
    ASSERT_EQ(domain.synchronize(), 0);
    ASSERT_TRUE(ptr.reset(&domain, new std::string("Goodbye, World!")));
    ASSERT_EQ(domain.synchronize(), 2);
    ASSERT_EQ(*ptr, "Goodbye, World!");
    ptr.reset();
    ASSERT_EQ(domain.synchronize(), 2);
    ASSERT_EQ(ptr.get(), nullptr);
    ASSERT_EQ(domain.synchronize(), 0);
}

TEST(RcuPtrTest, ConstructWeak) {
    sm::RcuDomain domain;
    sm::RcuSharedPtr<std::string> ptr = {&domain, new std::string("Hello, World!")};
    ASSERT_EQ(*ptr, "Hello, World!");
    sm::RcuWeakPtr<std::string> weak = ptr;
    ptr = nullptr;
    ASSERT_EQ(weak.lock(), nullptr);
}

TEST(RcuPtrTest, CompareExchangeStrong) {
    sm::RcuDomain domain;
    sm::RcuSharedPtr<std::string> ptr = {&domain, new std::string("Hello, World!")};
    ASSERT_EQ(*ptr, "Hello, World!");
    sm::RcuSharedPtr<std::string> ptr2 = ptr;
    ASSERT_EQ(*ptr2, "Hello, World!");

    sm::RcuSharedPtr<std::string> ptr3 = {&domain, new std::string("Goodbye, World!")};
    ASSERT_EQ(*ptr3, "Goodbye, World!");

    ASSERT_TRUE(ptr.compare_exchange_strong(ptr2, ptr3));
    ASSERT_EQ(*ptr, "Goodbye, World!");
    ASSERT_EQ(*ptr2, "Hello, World!");
    ASSERT_EQ(*ptr3, "Goodbye, World!");
}

TEST(RcuPtrTest, CompareExchangeStrongFail) {
    sm::RcuDomain domain;
    sm::RcuSharedPtr<std::string> ptr = {&domain, new std::string("Hello, World!")};
    ASSERT_EQ(*ptr, "Hello, World!");
    sm::RcuSharedPtr<std::string> ptr2 = ptr;
    ASSERT_EQ(*ptr2, "Hello, World!");

    sm::RcuSharedPtr<std::string> ptr3 = {&domain, new std::string("Goodbye, World!")};
    ASSERT_EQ(*ptr3, "Goodbye, World!");

    ASSERT_FALSE(ptr.compare_exchange_strong(ptr3, ptr3));
    ASSERT_EQ(*ptr, "Hello, World!");
    ASSERT_EQ(*ptr2, "Hello, World!");
    ASSERT_EQ(*ptr3, "Hello, World!");
}

TEST(RcuPtrTest, CompareExchangeWeak) {
    sm::RcuDomain domain;
    sm::RcuSharedPtr<std::string> ptr = {&domain, new std::string("Hello, World!")};
    ASSERT_EQ(*ptr, "Hello, World!");
    sm::RcuSharedPtr<std::string> ptr2 = ptr;
    ASSERT_EQ(*ptr2, "Hello, World!");

    sm::RcuSharedPtr<std::string> ptr3 = {&domain, new std::string("Goodbye, World!")};
    ASSERT_EQ(*ptr3, "Goodbye, World!");

    ASSERT_TRUE(ptr.compare_exchange_weak(ptr2, ptr3));
    ASSERT_EQ(*ptr, "Goodbye, World!");
    ASSERT_EQ(*ptr2, "Hello, World!");
    ASSERT_EQ(*ptr3, "Goodbye, World!");
}

TEST(RcuPtrTest, CompareExchangeWeakFail) {
    sm::RcuDomain domain;
    sm::RcuSharedPtr<std::string> ptr = {&domain, new std::string("Hello, World!")};
    ASSERT_EQ(*ptr, "Hello, World!");
    sm::RcuSharedPtr<std::string> ptr2 = ptr;
    ASSERT_EQ(*ptr2, "Hello, World!");

    sm::RcuSharedPtr<std::string> ptr3 = {&domain, new std::string("Goodbye, World!")};
    ASSERT_EQ(*ptr3, "Goodbye, World!");

    ASSERT_FALSE(ptr.compare_exchange_weak(ptr3, ptr3));
    ASSERT_EQ(*ptr, "Hello, World!");
    ASSERT_EQ(*ptr2, "Hello, World!");
    ASSERT_EQ(*ptr3, "Hello, World!");
}

TEST(RcuPtrTest, Exchange) {
    sm::RcuDomain domain;
    sm::RcuSharedPtr<std::string> ptr = {&domain, new std::string("Hello, World!")};
    ASSERT_EQ(*ptr, "Hello, World!");
    sm::RcuSharedPtr<std::string> ptr2 = ptr;
    ASSERT_EQ(*ptr2, "Hello, World!");

    sm::RcuSharedPtr<std::string> ptr3 = {&domain, new std::string("Goodbye, World!")};
    ASSERT_EQ(*ptr3, "Goodbye, World!");

    auto ptr4 = ptr.exchange(ptr3);
    ASSERT_EQ(*ptr, "Goodbye, World!");
    ASSERT_EQ(*ptr2, "Hello, World!");
    ASSERT_EQ(*ptr3, "Goodbye, World!");
    ASSERT_EQ(*ptr4, "Hello, World!");
}

TEST(RcuPtrTest, CopyWeak) {
    sm::RcuDomain domain;
    sm::RcuGuard guard(domain);
    sm::RcuSharedPtr<std::string> ptr = {&domain, new std::string("Hello, World!")};
    ASSERT_EQ(*ptr, "Hello, World!");
    sm::RcuSharedPtr<std::string> ptr2 = ptr;
    ASSERT_EQ(*ptr2, "Hello, World!");

    sm::RcuWeakPtr<std::string> weak = ptr;
    ASSERT_EQ(*weak.lock(), "Hello, World!");

    sm::RcuWeakPtr<std::string> weak2 = weak;
    ASSERT_EQ(*weak2.lock(), "Hello, World!");

    ptr = nullptr;
    ptr2 = nullptr;
    ASSERT_EQ(weak.lock(), nullptr);
    ASSERT_EQ(weak2.lock(), nullptr);
}

TEST(RcuPtrTest, ConcurrentAccess) {
    sm::RcuDomain domain;
    sm::RcuSharedPtr<std::string> ptr = {&domain, new std::string("Hello, World! i am a very long string to defeat small string optimizations.")};

    std::vector<std::jthread> threads;
    static constexpr size_t kThreadCount = 8;
    static constexpr size_t kIterationCount = 32;
    std::latch latch(kThreadCount + 1);

    std::jthread cleaner([&](std::stop_token stop_token) {
        latch.arrive_and_wait();

        while (!stop_token.stop_requested()) {
            domain.synchronize();
        }
    });

    for (size_t i = 0; i < kThreadCount; i++) {
        threads.emplace_back([&, i] {
            std::mt19937 mt(i);

            latch.arrive_and_wait();

            for (size_t i = 0; i < kIterationCount; i++) {
                {
                    sm::RcuGuard guard(domain);
                    // make a copy of the string
                    [[maybe_unused]] std::string str = auto{*ptr};
                }

                ptr = sm::rcuMakeShared<std::string>(&domain, RandomString(64, mt));
            }
        });
    }

    threads.clear();
    cleaner.request_stop();
    cleaner.join();

    SUCCEED();
}

using JointCount = sm::rcu::detail::JointCount;

TEST(JointCountTest, JointCount) {
    JointCount count { 1, 1 };

    ASSERT_TRUE(count.strongRetain()); // strong = 2, weak = 2
    ASSERT_TRUE(count.weakRetain()); // strong = 2, weak = 3
    ASSERT_EQ(count.strongRelease(), JointCount::eNone); // strong = 1, weak = 2
    ASSERT_FALSE(count.weakRelease()); // strong = 1, weak = 1
    ASSERT_EQ(count.strongRelease(), (JointCount::eStrong | JointCount::eWeak)); // strong = 0, weak = 0
}

TEST(JointCountTest, ReleaseStrongOnly) {
    JointCount count { 1, 1 };

    ASSERT_TRUE(count.strongRetain()); // strong = 2, weak = 2
    ASSERT_TRUE(count.weakRetain()); // strong = 2, weak = 3
    ASSERT_EQ(count.strongRelease(), JointCount::eNone); // strong = 1, weak = 2
    ASSERT_EQ(count.strongRelease(), JointCount::eStrong); // strong = 0, weak = 1
    ASSERT_TRUE(count.weakRelease());
}


TEST(JointCountTest, ReleaseWeakRepeat) {
    JointCount count { 1, 1 };

    ASSERT_TRUE(count.weakRetain()); // strong = 1, weak = 2
    ASSERT_TRUE(count.weakRetain()); // strong = 1, weak = 3
    ASSERT_TRUE(count.weakRetain()); // strong = 1, weak = 4
    ASSERT_FALSE(count.weakRelease()); // weak = 3
    ASSERT_FALSE(count.weakRelease()); // weak = 2
    ASSERT_FALSE(count.weakRelease()); // weak = 1
    ASSERT_TRUE(count.weakRelease()); // weak = 0
    ASSERT_FALSE(count.weakRelease()); // weak = 0 (sticky)
    ASSERT_FALSE(count.weakRelease()); // weak = 0 (sticky)
}

TEST(JointCountTest, ReleaseStrongRepeat) {
    JointCount count { 1, 1 };

    ASSERT_TRUE(count.weakRetain()); // strong = 1, weak = 2
    ASSERT_EQ(count.strongRelease(), JointCount::eStrong); // strong = 0, weak = 1
    ASSERT_FALSE(count.strongRetain()); // strong = 0, weak = 1
    ASSERT_TRUE(count.weakRelease()); // strong = 0, weak = 0
}

TEST(JointCountTest, ConcurrentAccess) {
    JointCount count { 1, 1 };

    std::atomic<int32_t> acquireStrongCount { 1024 };
    std::atomic<int32_t> releaseStrongCount { 1023 };
    std::atomic<int32_t> acquireWeakCount { 1024 };
    std::atomic<int32_t> releaseWeakCount { 1023 };

    std::vector<std::jthread> threads;
    static constexpr size_t kThreadCount = 8;
    std::latch latch(kThreadCount);
    std::latch acquireLatch(kThreadCount);
    std::latch releaseLatch(kThreadCount);

    for (size_t i = 0; i < kThreadCount; i++) {
        threads.emplace_back([&, i] {
            std::mt19937 mt(i);
            std::uniform_int_distribution<int> dist(0, 3);

            latch.arrive_and_wait();

            while (acquireStrongCount.fetch_sub(1) > 0) {
                count.strongRetain();
            }

            while (acquireWeakCount.fetch_sub(1) > 0) {
                count.weakRetain();
            }

            acquireLatch.arrive_and_wait();

            while (releaseStrongCount.fetch_sub(1) > 0) {
                count.strongRelease();
            }

            while (releaseWeakCount.fetch_sub(1) > 0) {
                count.weakRelease();
            }

            releaseLatch.arrive_and_wait();
        });
    }

    threads.clear();

    // weak = 3, strong = 2
    ASSERT_FALSE(count.weakRelease());
    // weak = 2, strong = 2
    ASSERT_EQ(count.strongRelease(), JointCount::eNone);
    // weak = 1, strong = 1
    ASSERT_EQ(count.strongRelease(), JointCount::eWeak | JointCount::eStrong);
}

TEST(JointCountTest, ConcurrentMixedAccess) {
    JointCount count { 1, 1 };

    std::vector<std::jthread> threads;
    static constexpr size_t kThreadCount = 8;
    static constexpr size_t kIterationCount = 1024;
    std::latch latch(kThreadCount);
    std::atomic<bool> die = false;

    for (size_t i = 0; i < kThreadCount; i++) {
        threads.emplace_back([&, i] {
            std::mt19937 mt(i);
            std::uniform_int_distribution<int> dist(0, 3);

            latch.arrive_and_wait();

            for (size_t i = 0; i < kIterationCount; i++) {
                if (!count.strongRetain()) die = true;
                if (!count.weakRetain()) die = true;
                if (!count.strongRetain()) die = true;
                if (!count.weakRetain()) die = true;
                if (count.strongRelease() != JointCount::eNone) die = true;
                if (count.weakRelease()) die = true;
            }
        });
    }

    threads.clear();

    ASSERT_FALSE(die);

    // weak = (kThreadCount * kIterationCount * 2) + 1, strong = (kThreadCount * kIterationCount) + 1
    for (size_t i = 0; i < (kThreadCount * kIterationCount); i++) {
        ASSERT_FALSE(count.weakRelease());
    }
    // weak = (kThreadCount * kIterationCount) + 1, strong = (kThreadCount * kIterationCount) + 1
    for (size_t i = 0; i < (kThreadCount * kIterationCount); i++) {
        ASSERT_EQ(count.strongRelease(), JointCount::eNone);
    }
    // weak = 1, strong = 1
    ASSERT_EQ(count.strongRelease(), JointCount::eWeak | JointCount::eStrong);
}

class IntrusiveBase : public sm::RcuIntrusivePtr<IntrusiveBase> {

};

class IntrusiveDerived : public IntrusiveBase {
    std::string mValue;
public:
    IntrusiveDerived(std::string value)
        : mValue(std::move(value))
    { }

    const std::string& value() const {
        return mValue;
    }
};

TEST(RcuPtrTest, IntrusiveDerived) {
    sm::RcuDomain domain;
    sm::RcuGuard guard(domain);
    sm::RcuSharedPtr<IntrusiveBase> ptr = sm::rcuMakeShared<IntrusiveDerived>(&domain, "Hello, World!");
    ASSERT_EQ(static_cast<IntrusiveDerived*>(ptr.get())->value(), "Hello, World!");
    sm::RcuWeakPtr<IntrusiveBase> weak = ptr;
    ptr = nullptr;
    ASSERT_EQ(weak.lock(), nullptr);
}
