#include "boot.h"

namespace mm
{
    void init(const array<E820Entry>& memory)
    {
        for(auto map : memory)
        {
            (void)map;
        }
    }
}