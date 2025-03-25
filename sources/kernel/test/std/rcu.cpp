#include <gtest/gtest.h>

#include <latch>
#include <random>
#include <thread>

#include "std/rcu.hpp"

TEST(RcuTest, Construct) {
    sm::RcuDomain domain;
}

struct Tree {
    std::atomic<Tree*> left;
    std::atomic<Tree*> right;

    std::atomic<uint64_t> data;
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
