#include <gtest/gtest.h>

#include "allocator/allocator.hpp"

#include "std/vector.hpp"

class TestAllocator : public mem::IAllocator {
public:
    void* allocate(size_t size, size_t _) override {
        void *ptr = std::malloc(size);
        std::cerr << "allocating " << size << " bytes at " << (uintptr_t)ptr << std::endl;
        return ptr;
    }

    void* reallocate(void* old, size_t _, size_t newSize) override {
        std::cerr << "reallocating " << (uintptr_t)old << " to " << newSize << std::endl;
        return std::realloc(old, newSize);
    }

    void deallocate(void* ptr, size_t _) override {
        std::cerr << "deallocating " << (uintptr_t)ptr << std::endl;
        std::free(ptr);
    }
};

TEST(VectorTest, DefaultConstructor) {
    TestAllocator allocator;
    stdx::Vector<int> vec(&allocator);
    ASSERT_TRUE(vec.isEmpty());
    ASSERT_EQ(vec.count(), 0);
}

TEST(VectorTest, ConstructorWithCapacity) {
    TestAllocator allocator;
    stdx::Vector<int> vec(&allocator, 10);
    ASSERT_TRUE(vec.isEmpty());
    ASSERT_EQ(vec.count(), 0);
    ASSERT_EQ(vec.capacity(), 10);
}

TEST(VectorTest, ConstructorWithSpan) {
    TestAllocator allocator;
    int data[] = {1, 2, 3, 4, 5};
    std::span<const int> span(data, 5);
    stdx::Vector<int> vec(&allocator, span);
    ASSERT_FALSE(vec.isEmpty());
    ASSERT_EQ(vec.count(), 5);
    ASSERT_GE(vec.capacity(), 5);
    for (size_t i = 0; i < 5; ++i) {
        ASSERT_EQ(vec[i], data[i]);
    }
}

TEST(VectorTest, CopyConstructor) {
    TestAllocator allocator;
    stdx::Vector<int> vec1(&allocator, 5);
    vec1.add(1);
    vec1.add(2);
    vec1.add(3);
    stdx::Vector<int> vec2(vec1);
    ASSERT_EQ(vec2.count(), 3);
    ASSERT_GE(vec2.capacity(), 3);
    for (size_t i = 0; i < 3; ++i) {
        ASSERT_EQ(vec2[i], vec1[i]);
    }
}

TEST(VectorTest, MoveConstructor) {
    TestAllocator allocator;
    stdx::Vector<int> vec1(&allocator, 5);
    vec1.add(1);
    vec1.add(2);
    vec1.add(3);
    stdx::Vector<int> vec2(std::move(vec1));
    ASSERT_EQ(vec2.count(), 3);
    ASSERT_EQ(vec2.capacity(), 5);
    ASSERT_TRUE(vec1.isEmpty());
}

TEST(VectorTest, Add) {
    TestAllocator allocator;
    stdx::Vector<int> vec(&allocator, 2);
    vec.add(1);
    vec.add(2);
    vec.add(3);
    ASSERT_EQ(vec.count(), 3);
    ASSERT_GE(vec.capacity(), 3);
    ASSERT_EQ(vec[0], 1);
    ASSERT_EQ(vec[1], 2);
    ASSERT_EQ(vec[2], 3);
}

TEST(VectorTest, AddRange) {
    TestAllocator allocator;
    stdx::Vector<int> vec(&allocator, 2);
    int data[] = {1, 2, 3, 4, 5};
    std::span<const int> span(data, 5);
    vec.addRange(span);
    ASSERT_EQ(vec.count(), 5);
    ASSERT_GE(vec.capacity(), 5);
    for (size_t i = 0; i < 5; ++i) {
        ASSERT_EQ(vec[i], data[i]);
    }
}
