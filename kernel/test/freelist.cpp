#include <gtest/gtest.h>
#include "allocator/freelist.hpp"

class FreeVectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize FreeVector with some blocks
        freeVector.freeBlock({200, 100});
        freeVector.freeBlock({400, 100});
        freeVector.freeBlock({600, 100});
    }

    std::unique_ptr<mem::FreeBlock[]> blocks = std::make_unique<mem::FreeBlock[]>(256);
    mem::FreeVector freeVector = {blocks.get(), blocks.get() + 256};
};

TEST_F(FreeVectorTest, FindBlockExactSize) {
    auto block = freeVector.findBlock(100, 1);
    EXPECT_EQ(block.offset, 200);
    EXPECT_EQ(block.length, 100);
}

TEST_F(FreeVectorTest, FindBlockWithAlignment) {
    auto block = freeVector.findBlock(100, 200);
    EXPECT_EQ(block.offset, 200);
    EXPECT_EQ(block.length, 100);
}

TEST_F(FreeVectorTest, FindBlockWithPadding) {
    auto block = freeVector.findBlock(50, 64);
    EXPECT_EQ(block.offset, 448);
    EXPECT_EQ(block.length, 50);
}

TEST_F(FreeVectorTest, FindBlockNotEnoughSpace) {
    auto block = freeVector.findBlock(150, 1);
    ASSERT_TRUE(block.isEmpty());
}

TEST_F(FreeVectorTest, FindBlockAfterAllocation) {
    freeVector.findBlock(100, 1); // Allocate first block
    auto block = freeVector.findBlock(100, 1);
    EXPECT_EQ(block.offset, 400);
    EXPECT_EQ(block.length, 100);
}

TEST_F(FreeVectorTest, CompactFunction) {
    freeVector.freeBlock({300, 100});
    freeVector.compact();
    ASSERT_EQ(freeVector.count(), 2);

    auto block = freeVector.findBlock(300, 1);
    EXPECT_EQ(block.offset, 200);
    EXPECT_EQ(block.length, 300);
}

TEST_F(FreeVectorTest, CompactFunctionWithGaps) {
    freeVector.freeBlock({100, 50});
    freeVector.freeBlock({150, 50});
    freeVector.compact();
    ASSERT_EQ(freeVector.count(), 3);

    auto block = freeVector.findBlock(200, 1);
    EXPECT_EQ(block.offset, 100);
    EXPECT_EQ(block.length, 200);
}

TEST_F(FreeVectorTest, CompactFunctionNoChange) {
    freeVector.compact();
    ASSERT_EQ(freeVector.count(), 3);

    auto block = freeVector.findBlock(100, 1);
    EXPECT_EQ(block.offset, 200);
    EXPECT_EQ(block.length, 100);
    block = freeVector.findBlock(100, 1);
    EXPECT_EQ(block.offset, 400);
    EXPECT_EQ(block.length, 100);
    block = freeVector.findBlock(100, 1);
    EXPECT_EQ(block.offset, 600);
    EXPECT_EQ(block.length, 100);

    // more allocations should fail

    block = freeVector.findBlock(100, 1);
    ASSERT_TRUE(block.isEmpty());
}
