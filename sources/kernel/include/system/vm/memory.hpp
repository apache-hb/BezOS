#pragma once

#include "system/vmem.hpp"

namespace sys {
    /// @brief Physical storage for vm objects.
    ///
    /// This contains physical memory that a vm segment maps onto,
    /// multiple vm objects may map to the same memory segment.
    /// This object can be referenced from multiple processes at the same time.
    class MemoryObject : public IMemoryObject {
        km::MemoryRange mRange;

    public:
        MemoryObject(km::MemoryRange range)
            : mRange(range)
        { }

        OsStatus release(size_t) override {
            return OsStatusSuccess;
        }

        km::MemoryRange range() override {
            return mRange;
        }
    };

    OsStatus NewMemoryObject(sm::RcuDomain *domain, km::MemoryRange range, sm::RcuSharedPtr<MemoryObject> *outObject);
}
