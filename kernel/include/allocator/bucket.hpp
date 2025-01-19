#pragma once

#include "allocator/allocator.hpp"

#include "std/static_vector.hpp"

namespace mem {
    struct Bucket {
        uint64_t width;
        IAllocator *allocator;
    };

    class BucketAllocator final : public mem::IAllocator {
        stdx::StaticVector<Bucket, 4> mBuckets;

        Bucket *getBucketFor(size_t size) {
            for (auto& bucket : mBuckets) {
                if (size <= bucket.width) {
                    return &bucket;
                }
            }

            return &mBuckets.back();
        }

    public:
        BucketAllocator(stdx::StaticVector<Bucket, 4> buckets)
            : mBuckets(buckets)
        { }

        void *allocate(size_t size) override {
            Bucket *bucket = getBucketFor(size);
            do {
                void *ptr = bucket->allocator->allocate(size);
                if (ptr)
                    return ptr;

            } while (++bucket != mBuckets.end());

            return nullptr;
        }

        void *allocateAligned(size_t size, size_t align = sizeof(std::max_align_t)) override {
            Bucket *bucket = getBucketFor(size);
            do {
                void *ptr = bucket->allocator->allocateAligned(size, align);
                if (ptr)
                    return ptr;

            } while (++bucket != mBuckets.end());

            return nullptr;
        }

        void deallocate(void *ptr, size_t size) override {
            Bucket *bucket = getBucketFor(size);

            // TOOD: this isnt great
            do {
                bucket->allocator->deallocate(ptr, size);
            } while (++bucket != mBuckets.end());
        }
    };
}
