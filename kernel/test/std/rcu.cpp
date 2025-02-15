#include <gtest/gtest.h>

#include "std/rcu.hpp"

TEST(RcuTest, Construct) {
    sm::RcuDomain<std::string> domain;
}

struct Tree {
    Tree *left;
    Tree *right;

    uint8_t data[0x1000];
};
