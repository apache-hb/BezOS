#pragma once

#include "process/object.hpp"

namespace km {
    struct Mutex : public BaseObject {
        stdx::SpinLock lock;

        Mutex(MutexId id, stdx::String name)
            : BaseObject(std::to_underlying(id) | (uint64_t(eOsHandleMutex) << 56), std::move(name))
        { }
    };
}
