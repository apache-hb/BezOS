#pragma once

#include "kernel/types.h"

namespace bezos::fs
{
    using offset_t = u64;
    
    struct stream
    {
        virtual void write(u8* ptr, u64 len) = 0;
        virtual void read(u8* ptr, u64 len) = 0;
        virtual void seek(offset_t offset) = 0;
        virtual offset_t pos() const = 0;
    };
}