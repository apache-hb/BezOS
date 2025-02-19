#pragma once

#include <gtest/gtest.h>

#include "acpi/aml.hpp"

#include "allocator/bucket.hpp"

struct AmlTestContext {
    static constexpr size_t kBufferSize0 = 0x10000;
    static constexpr size_t kBufferSize1 = 0x10000;
    static constexpr size_t kBufferSize2 = 0x100000;
    std::unique_ptr<uint8_t[]> buffer0{new uint8_t[kBufferSize0]};
    std::unique_ptr<uint8_t[]> buffer1{new uint8_t[kBufferSize1]};
    std::unique_ptr<uint8_t[]> buffer2{new uint8_t[kBufferSize2]};

    mem::BitmapAllocator arena0 { buffer0.get(), buffer0.get() + kBufferSize0 };
    mem::BitmapAllocator arena1 { buffer1.get(), buffer1.get() + kBufferSize1 };
    mem::BitmapAllocator arena2 { buffer2.get(), buffer2.get() + kBufferSize2 };

    mem::BucketAllocator arena { {
        { .width = 32, .allocator = &arena0 },
        { .width = 256, .allocator = &arena1 },
        { .width = 512, .allocator = &arena2 },
    } };
};
