#pragma once

#include <cstddef>

#include "allocator/allocator.hpp"

#include <tlsf.h>

namespace mem {
    class TlsfAllocator : public mem::IAllocator {
        using Super = mem::IAllocator;

        using TlsfHandle = std::unique_ptr<void, decltype(&tlsf_destroy)>;

        TlsfHandle mAllocator;

    public:
        using Super::Super;

        tlsf_t getAllocator() const {
            return mAllocator.get();
        }

        constexpr TlsfAllocator()
            : mAllocator(nullptr, tlsf_destroy)
        { }

        constexpr TlsfAllocator(void *memory, size_t size)
            : mAllocator(tlsf_create_with_pool(memory, size), tlsf_destroy)
        { }

        constexpr TlsfAllocator(tlsf_t allocator)
            : mAllocator(allocator, tlsf_destroy)
        { }

        pool_t addPool(void *memory, size_t size) [[clang::allocating]] {
            return tlsf_add_pool(mAllocator.get(), memory, size);
        }

        void removePool(pool_t pool) [[clang::allocating]] {
            tlsf_remove_pool(mAllocator.get(), pool);
        }

        void *allocate(size_t size) [[clang::allocating]] override {
            return tlsf_malloc(mAllocator.get(), size);
        }

        void *allocateAligned(size_t size, size_t align = alignof(std::max_align_t)) [[clang::allocating]] override {
            return tlsf_memalign(mAllocator.get(), align, size);
        }

        void *reallocate(void *old, size_t _, size_t newSize) [[clang::allocating]] override {
            return tlsf_realloc(mAllocator.get(), old, newSize);
        }

        void deallocate(void *ptr, size_t _) noexcept [[clang::nonallocating]] override {
            tlsf_free(mAllocator.get(), ptr);
        }
    };
}
