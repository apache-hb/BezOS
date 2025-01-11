#pragma once

#include <cstddef>

#include "allocator/allocator.hpp"

#include <tlsf.h>

namespace mem {
    class TlsfAllocator final : public mem::IAllocator {
        using Super = mem::IAllocator;

        tlsf_t mAllocator;

    public:
        using Super::Super;

        constexpr TlsfAllocator(void *memory, size_t size)
            : mAllocator(tlsf_create_with_pool(memory, size, 0))
        { }

        ~TlsfAllocator() {
            tlsf_destroy(mAllocator);
        }

        constexpr TlsfAllocator(const TlsfAllocator&) = delete;
        TlsfAllocator& operator=(const TlsfAllocator&) = delete;

        constexpr TlsfAllocator(TlsfAllocator&&) = delete;
        TlsfAllocator& operator=(TlsfAllocator&&) = delete;

        void *allocate(size_t size, size_t _) override {
            return tlsf_malloc(mAllocator, size);
        }

        void *reallocate(void *old, size_t _, size_t newSize) override {
            return tlsf_realloc(mAllocator, old, newSize);
        }

        void deallocate(void *ptr, size_t _) override {
            tlsf_free(mAllocator, ptr);
        }
    };
}
