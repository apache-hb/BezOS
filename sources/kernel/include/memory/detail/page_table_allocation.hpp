#pragma once

#include "arch/paging.hpp"
#include "common/physical_address.hpp"

#include <stddef.h>
#include <stdint.h>

namespace km {
    class PageTableAllocation {
        void *mBlock;
        size_t mSlide;

    public:
        PageTableAllocation(void *block = nullptr, size_t slide = 0) noexcept [[clang::nonblocking]]
            : mBlock(block)
            , mSlide(slide)
        { }

        void *getVirtual() const noexcept [[clang::nonblocking]] {
            return mBlock;
        }

        sm::PhysicalAddress getPhysical() const noexcept [[clang::nonblocking]] {
            return sm::PhysicalAddress { (uintptr_t)mBlock - mSlide };
        }

        size_t getSlide() const noexcept [[clang::nonblocking]] {
            return mSlide;
        }

        bool isPresent() const noexcept [[clang::nonblocking]] {
            return mBlock != nullptr;
        }

        void setNext(PageTableAllocation next) noexcept [[clang::nonblocking]] {
            memcpy(mBlock, &next, sizeof(PageTableAllocation));
        }

        PageTableAllocation getNext() const noexcept [[clang::nonblocking]] {
            PageTableAllocation next;
            memcpy(&next, mBlock, sizeof(PageTableAllocation));
            return next;
        }

        PageTableAllocation offset(size_t index) const noexcept [[clang::nonblocking]] {
            void *ptr = (void*)((uintptr_t)mBlock + (index * x64::kPageSize));
            return PageTableAllocation { ptr, mSlide };
        }

        operator bool() const noexcept [[clang::nonblocking]] {
            return isPresent();
        }
    };
}
