#pragma once

#include "system/vmem.hpp"

namespace sys {
    /// @brief Virtual memory in a single process address space.
    class VmObject {
        km::VirtualRange mRange;
        sm::RcuSharedPtr<IMemoryObject> mBacking;

    public:
        VmObject(km::VirtualRange range, sm::RcuSharedPtr<IMemoryObject> backing)
            : mRange(range)
            , mBacking(backing)
        { }

        km::VirtualRange vmRange() const noexcept { return mRange; }
    };
}
