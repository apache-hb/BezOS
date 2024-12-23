#pragma once

#include <stddef.h>

namespace stdx {
    template<typename T, size_t N>
    class StaticFifo {
        T mStorage[N];
        size_t mHead;
        size_t mTail;

    public:
        StaticFifo();
    };
}