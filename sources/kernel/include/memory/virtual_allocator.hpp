#pragma once

#include "arch/paging.hpp"
#include "memory/range.hpp"
#include "memory/range_allocator.hpp"

namespace km {
    class VmemAllocator {
        RangeAllocator<const std::byte*> mVmemAllocator;
    public:
        VmemAllocator() = default;
        VmemAllocator(VirtualRange vmemArea)
            : mVmemAllocator(vmemArea.cast<const std::byte*>())
        { }

        void reserve(VirtualRange range) {
            mVmemAllocator.reserve(range.cast<const std::byte*>());
        }

        VirtualRange allocate(size_t pages, const void *hint = nullptr) {
            size_t align = (pages > (x64::kLargePageSize / x64::kPageSize)) ? x64::kLargePageSize : x64::kPageSize;
            return allocate({ .size = pages * x64::kPageSize, .align = align, .hint = hint });
        }

        VirtualRange allocate(RangeAllocateRequest<const void*> request) {
            auto range = mVmemAllocator.allocate({ .size = request.size, .align = request.align, .hint = (const std::byte*)request.hint });
            return range.cast<const void*>();
        }

        void release(VirtualRange range) {
            mVmemAllocator.release(range.cast<const std::byte*>());
        }
    };
}
