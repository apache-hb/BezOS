#pragma once

#include "allocator/freelist.hpp"
#include "memory/virtual_allocator.hpp"

namespace mem {
    class VmemAllocator {
        FreeVector mFreeBlocks;
        km::VirtualRange mRange;

    public:
        VmemAllocator(FreeVector blocks, km::VirtualRange range)
            : mFreeBlocks(blocks)
            , mRange(range)
        { }


    };
}
