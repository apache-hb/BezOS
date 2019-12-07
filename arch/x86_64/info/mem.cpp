#include "mem.h"

extern "C" void internal_memory_size(bezos::addr_range_desc* ptr, bezos::u32 len);

namespace bezos
{
    struct addr_range_desc
    {
        u64 addr;
        u64 length;
        u32 type;
        u32 attrib;
    };

    static_assert(sizeof(addr_range_desc) == 24);

    u64 memory_size()
    {
        addr_range_desc buf[32];
        internal_memory_size(buf, sizeof(buf));
    }
}
