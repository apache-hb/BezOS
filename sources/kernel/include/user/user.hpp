#pragma once

#include <bezos/status.h>

#include "memory/pte.hpp"

#include <cstddef>

namespace km {
    bool IsPageMapped(PageTables& pt, const void *address, PageFlags flags);

    bool IsRangeMapped(PageTables& pt, const void *begin, const void *end, PageFlags flags);

    OsStatus CopyUserMemory(PageTables& pt, uint64_t address, size_t size, void *copy);

    OsStatus ReadUserMemory(PageTables& pt, const void *front, const void *back, void *dst, size_t size);

    template<typename Range>
    OsStatus CopyUserRange(PageTables& ptes, const void *front, const void *back, Range *dst, size_t limit) {
        if (front == back) {
            return OsStatusSuccess;
        }

        if (front > back) {
            return OsStatusInvalidSpan;
        }

        size_t size = (uintptr_t)back - (uintptr_t)front;
        if (size > limit) {
            return OsStatusInvalidSpan;
        }

        if (!IsRangeMapped(ptes, front, back, PageFlags::eUser | PageFlags::eRead)) {
            return OsStatusInvalidAddress;
        }

        dst->resize(size);
        memcpy(dst->data(), front, size);
        return OsStatusSuccess;
    }

    OsStatus WriteUserMemory(PageTables& pt, void *dst, const void *src, size_t size);
}
