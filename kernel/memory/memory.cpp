#include "memory.h"

#include "arch/x86_64/info/mem.h"
#include "kernel/kernel.h"

namespace bezos::memory
{
    void init()
    {
        auto m = memory_size();
        print(m);
    }
}