#pragma once

#include "common/util/util.hpp"

#include <gtest/gtest.h>
#include <random>
#include <stdlib.h>

enum AllocFlags {
    eNone = 0,
    eNoThrow = (1 << 0),
    eArray = (1 << 1),
};

UTIL_BITFLAGS(AllocFlags);

class TestAllocator {
    bool shouldAllocFail() {
        if (mAllocCount > mFailAfter) {
            return true;
        }

        mAllocCount += 1;

        if (mFailPercent > 0.0f) {
            float randomValue = mDistribution(mRandom);
            if (randomValue < mFailPercent) {
                return true;
            }
        }

        return false;
    }

public:
    void *operatorNew(size_t size, size_t align, AllocFlags flags) {
        if (mNoAlloc) {
            fprintf(stderr, "Allocation occurred when no alloc was expected\n");
            abort();
        }

        if (shouldAllocFail()) {
            return nullptr;
        }

        if (void *ptr = aligned_alloc(align, size)) {
            return ptr;
        }

        if (bool(flags & eNoThrow)) {
            return nullptr;
        }

        throw std::bad_alloc();
    }

    void operatorDelete(void *ptr, [[maybe_unused]] size_t size, [[maybe_unused]] size_t align, [[maybe_unused]] AllocFlags flags) {
        if (ptr == nullptr) {
            return;
        }

        mFreeCount += 1;

        free(ptr);
    }

    void reset(size_t seed = 0x1234) {
        mAllocCount = 0;
        mFreeCount = 0;
        mNoAlloc = false;
        mFailAfter = SIZE_MAX;
        mFailPercent = 0.0f;
        mRandom.seed(seed);
        mDistribution = std::uniform_real_distribution<float>(0.0f, 1.0f);
    }

    size_t mFailAfter = SIZE_MAX;
    bool mNoAlloc = false;

    size_t mAllocCount = 0;
    size_t mFreeCount = 0;

    float mFailPercent = 0.0f;
    std::mt19937 mRandom;
    std::uniform_real_distribution<float> mDistribution { 0.0f, 1.0f };
};

TestAllocator *GetGlobalAllocator();
