#pragma once

#include "common/util/util.hpp"

#include <unistd.h>

namespace pv {
    class Memory {
        int mMemoryFd;
        void *mHostMemory;
        off64_t mMemorySize;

    public:
        UTIL_NOCOPY(Memory);
        UTIL_NOMOVE(Memory);

        Memory(off64_t size);
        ~Memory();

        off64_t getMemorySize() const noexcept {
            return mMemorySize;
        }
    };
}
