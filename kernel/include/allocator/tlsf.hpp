#pragma once

#include <cstddef>

#include "allocator/allocator.hpp"
#include "log.hpp"

#include <tlsf.h>

namespace mem {
    class TlsfAllocator : public mem::IAllocator {
        using Super = mem::IAllocator;

        using TlsfHandle = std::unique_ptr<void, void(*)(tlsf_t)>;

        TlsfHandle mAllocator;

    public:
        using Super::Super;

        constexpr TlsfAllocator()
            : mAllocator(nullptr, tlsf_destroy)
        { }

        constexpr TlsfAllocator(void *memory, size_t size)
            : mAllocator(tlsf_create_with_pool(memory, size), tlsf_destroy)
        { }

        constexpr TlsfAllocator(tlsf_t allocator)
            : mAllocator(allocator, tlsf_destroy)
        { }

        pool_t addPool(void *memory, size_t size) {
            return tlsf_add_pool(mAllocator.get(), memory, size);
        }

        void removePool(pool_t pool) {
            tlsf_remove_pool(mAllocator.get(), pool);
        }

        void *allocate(size_t size) override {
            void *ptr = tlsf_malloc(mAllocator.get(), size);
            if (ptr == nullptr) {
                KmDebugMessage("Failed to allocate ", size, " bytes\n");
            }
            return ptr;
        }

        void *allocateAligned(size_t size, size_t align = alignof(std::max_align_t)) override {
            void *ptr = tlsf_memalign(mAllocator.get(), align, size);
            if (ptr == nullptr) {
                KmDebugMessage("Failed to allocate aligned ", size, " bytes\n");
            }
            return ptr;
        }

        void *reallocate(void *old, size_t oldSize, size_t newSize) override {
            void *ptr = tlsf_realloc(mAllocator.get(), old, newSize);
            if (ptr == nullptr) {
                KmDebugMessage("Failed to reallocate ", newSize, " bytes, ", old, ", ", oldSize, ", ", mAllocator.get(), "\n");
            }
            return ptr;
        }

        void deallocate(void *ptr, size_t _) override {
            tlsf_free(mAllocator.get(), ptr);
        }
    };
}
