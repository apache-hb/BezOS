#pragma once

#include <cstddef>

#include "allocator/allocator.hpp"

#include <tlsf.h>

namespace mem {
    class TlsfAllocator : public mem::IAllocator {
        using Super = mem::IAllocator;

        using TlsfHandle = std::unique_ptr<void, void(*)(tlsf_t)>;

        TlsfHandle mAllocator;

    public:
        using Super::Super;

        constexpr TlsfAllocator(void *memory, size_t size)
            : mAllocator(tlsf_create_with_pool(memory, size, 0), tlsf_destroy)
        { }

        void *allocate(size_t size) override {
            return tlsf_malloc(mAllocator.get(), size);
        }

        void *allocateAligned(size_t size, size_t align = alignof(std::max_align_t)) override {
            return tlsf_memalign(mAllocator.get(), size, align);
        }

        void *reallocate(void *old, size_t _, size_t newSize) override {
            return tlsf_realloc(mAllocator.get(), old, newSize);
        }

        void deallocate(void *ptr, size_t _) override {
            tlsf_free(mAllocator.get(), ptr);
        }
    };
}
