#include "user/user.hpp"

bool km::IsPageMapped(PageTables& pt, const void *address, km::PageFlags flags) {
    return (pt.getMemoryFlags(address) & flags) == flags;
}

bool km::IsRangeMapped(PageTables& pt, const void *begin, const void *end, km::PageFlags flags) {
    uintptr_t front = (uintptr_t)begin;
    uintptr_t back = (uintptr_t)end;

    const PageBuilder *pm = pt.pageManager();

    if (!pm->isCanonicalAddress(begin) || !pm->isCanonicalAddress(end)) {
        return false;
    }

    for (uintptr_t addr = front; addr < back; addr += x64::kPageSize) {
        if (!IsPageMapped(pt, (void*)addr, flags)) {
            return false;
        }
    }

    return true;
}

OsStatus km::CopyUserMemory(PageTables& pt, uint64_t address, size_t size, void *copy) {
    uint64_t tail = address;
    if (__builtin_add_overflow(address, size, &tail)) {
        return OsStatusInvalidSpan;
    }

    const void *front = (void*)address;
    const void *back = (void*)tail;

    if (!IsRangeMapped(pt, front, back, PageFlags::eUser | PageFlags::eRead)) {
        return OsStatusInvalidAddress;
    }

    memcpy(copy, front, size);
    return OsStatusSuccess;
}

OsStatus km::ReadUserMemory(PageTables& pt, const void *front, const void *back, void *dst, size_t size) {
    if (front >= back) {
        return OsStatusInvalidSpan;
    }

    size_t range = (uintptr_t)back - (uintptr_t)front;
    if (size > range) {
        return OsStatusInvalidSpan;
    }

    if (!IsRangeMapped(pt, front, back, PageFlags::eUser | PageFlags::eRead)) {
        return OsStatusInvalidAddress;
    }

    memcpy(dst, front, size);
    return OsStatusSuccess;
}

OsStatus km::WriteUserMemory(PageTables& pt, void *dst, const void *src, size_t size) {
    uintptr_t back = (uintptr_t)dst;
    if (__builtin_add_overflow(back, size, &back)) {
        return OsStatusInvalidSpan;
    }

    if (!IsRangeMapped(pt, dst, (void*)back, PageFlags::eUser | PageFlags::eWrite)) {
        return OsStatusInvalidAddress;
    }

    memcpy(dst, src, size);
    return OsStatusSuccess;
}
