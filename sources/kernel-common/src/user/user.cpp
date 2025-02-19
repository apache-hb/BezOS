#include "user/user.hpp"

bool km::IsPageMapped(const km::PageTableManager& pt, const void *address, km::PageFlags flags) {
    return (pt.getMemoryFlags(address) & flags) == flags;
}

bool km::IsRangeMapped(const km::PageTableManager& pt, const void *begin, const void *end, km::PageFlags flags) {
    uintptr_t front = (uintptr_t)begin;
    uintptr_t back = (uintptr_t)end;

    for (uintptr_t addr = front; addr < back; addr += x64::kPageSize) {
        if (!IsPageMapped(pt, (void*)addr, flags)) {
            return false;
        }
    }

    return true;
}

OsStatus km::CopyUserMemory(const km::PageTableManager& pt, uint64_t address, size_t size, void *copy) {
    uint64_t tail = address;
    if (__builtin_add_overflow(address, size, &tail)) {
        return OsStatusInvalidInput;
    }

    const void *front = (void*)address;
    const void *back = (void*)tail;

    if (!IsRangeMapped(pt, front, back, PageFlags::eUser | PageFlags::eRead)) {
        return OsStatusInvalidInput;
    }

    memcpy(copy, front, size);
    return OsStatusSuccess;
}

OsStatus km::ReadUserMemory(const km::PageTableManager& pt, const void *front, const void *back, void *dst, size_t size) {
    if (front >= back) {
        return OsStatusInvalidInput;
    }

    size_t range = (uintptr_t)back - (uintptr_t)front;
    if (size > range) {
        return OsStatusInvalidInput;
    }

    if (!IsRangeMapped(pt, front, back, PageFlags::eUser | PageFlags::eRead)) {
        return OsStatusInvalidInput;
    }

    memcpy(dst, front, size);
    return OsStatusSuccess;
}

OsStatus km::WriteUserMemory(const km::PageTableManager& pt, void *dst, const void *src, size_t size) {
    uintptr_t back = (uintptr_t)dst;
    if (__builtin_add_overflow(back, size, &back)) {
        return OsStatusInvalidInput;
    }

    if (!IsRangeMapped(pt, dst, (void*)back, PageFlags::eUser | PageFlags::eWrite)) {
        return OsStatusInvalidInput;
    }

    memcpy(dst, src, size);
    return OsStatusSuccess;
}
