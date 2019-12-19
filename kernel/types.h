#pragma once

namespace bezos
{
    using u8 = unsigned char;
    static_assert(sizeof(u8) == 1);

    using u16 = unsigned short;
    static_assert(sizeof(u16) == 2);

    using u32 = unsigned int;
    static_assert(sizeof(u32) == 4);

    using u64 = unsigned long long;
    static_assert(sizeof(u64) == 8);

    union addr_t
    {
        u64 addr;
        struct parts
        {
            u32 low;
            u32 high;
        };
    };

    static_assert(sizeof(addr_t) == 8);
}