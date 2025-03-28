#pragma once

#include <bezos/status.h>

#include "arch/paging.hpp"

#include "memory/memory.hpp"

namespace km {
    class PageTables;

    struct AllocateRequest {
        size_t size;
        size_t align = x64::kPageSize;
        const void *hint = nullptr;
        PageFlags flags = PageFlags::eNone;
        MemoryType type = MemoryType::eWriteBack;
    };

    void copyHigherHalfMappings(PageTables *tables, const PageTables *source);
}
