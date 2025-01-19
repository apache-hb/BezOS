#include <gtest/gtest.h>

#include <stdint.h>
#include <unordered_set>

#include "allocator/bitmap.hpp"

static constexpr size_t kSize = 0x10000;

static auto GetAllocatorMemory() {
    auto deleter = [](uint8_t *ptr) {
        :: operator delete[] (ptr, std::align_val_t(16));
    };

    return std::unique_ptr<uint8_t[], decltype(deleter)>{
        new (std::align_val_t(16)) uint8_t[kSize],
        deleter
    };
}

TEST(BitmapAllocTest, Construction) {
    std::unique_ptr memory = GetAllocatorMemory();

    mem::BitmapAllocator bitmap {memory.get(), memory.get() + kSize, 16};
}

TEST(BitmapAllocTest, Allocate) {
    std::unique_ptr memory = GetAllocatorMemory();

    mem::BitmapAllocator bitmap {memory.get(), memory.get() + kSize, 16};

    void *ptr = bitmap.allocateAligned(99, 16);
    ASSERT_NE(ptr, nullptr);

    void *ptr2 = bitmap.allocateAligned(99, 16);
    ASSERT_NE(ptr2, nullptr);

    void *ptr3 = bitmap.allocateAligned(999, 16);
    ASSERT_NE(ptr3, nullptr);

    ASSERT_NE(ptr, ptr2);
    ASSERT_NE(ptr, ptr3);

    bitmap.deallocate(ptr3, 999);
    bitmap.deallocate(ptr2, 99);
    bitmap.deallocate(ptr, 99);
}

TEST(BitmapAllocTest, OverAllocate) {
    std::unique_ptr memory = GetAllocatorMemory();

    mem::BitmapAllocator bitmap {memory.get(), memory.get() + kSize, 16};

    void *ptr = bitmap.allocateAligned(kSize * 2, 16);
    ASSERT_EQ(ptr, nullptr);

    ptr = bitmap.allocateAligned(kSize + 1, 16);
    ASSERT_EQ(ptr, nullptr);

    ptr = bitmap.allocateAligned(kSize, 16);
    ASSERT_NE(ptr, nullptr);

    void *ptr2 = bitmap.allocateAligned(1, 16);
    ASSERT_EQ(ptr2, nullptr);

    bitmap.deallocate(ptr, kSize);

    ptr2 = bitmap.allocateAligned(1, 16);
    ASSERT_NE(ptr2, nullptr);
}

TEST(BitmapAllocTest, NoDuplicates) {
    std::unique_ptr memory = GetAllocatorMemory();

    mem::BitmapAllocator bitmap {memory.get(), memory.get() + kSize, 16};

    std::unordered_set<void*> pointers;

    for (size_t i = 0; i < (kSize / 32); i++) {
        void *ptr = bitmap.allocateAligned(32, 16);
        ASSERT_NE(ptr, nullptr);

        auto [it, inserted] = pointers.insert(ptr);
        ASSERT_TRUE(inserted);
    }
}
