#pragma once

#include <bezos/status.h>

#include "memory/allocator.hpp"

#include <cstddef>

namespace km {
    bool IsPageMapped(const km::PageTableManager& pt, const void *address, km::PageFlags flags);

    bool IsRangeMapped(const km::PageTableManager& pt, const void *begin, const void *end, km::PageFlags flags);

    OsStatus CopyUserMemory(const km::PageTableManager& pt, uint64_t address, size_t size, void *copy);

    OsStatus ReadUserMemory(const km::PageTableManager& pt, const void *front, const void *back, void *dst, size_t size);

    template<typename Range>
    OsStatus CopyUserRange(const km::PageTableManager& pt, const void *front, const void *back, Range *dst, size_t limit) {
        if (front >= back) {
            return OsStatusInvalidInput;
        }

        size_t size = (uintptr_t)back - (uintptr_t)front;
        if (size > limit) {
            return OsStatusInvalidInput;
        }

        if (!IsRangeMapped(pt, front, back, PageFlags::eUser | PageFlags::eRead)) {
            return OsStatusInvalidInput;
        }

        dst->resize(size);
        memcpy(dst->data(), front, size);
        return OsStatusSuccess;
    }


    OsStatus WriteUserMemory(const km::PageTableManager& pt, void *dst, const void *src, size_t size);
}
