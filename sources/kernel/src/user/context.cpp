#include "user/context.hpp"

#include "memory/paging.hpp"
#include "memory/pte.hpp"

static constexpr bool IsAlignedTo(uint64_t address, size_t alignment) {
    return (address & (alignment - 1)) == 0;
}

OsStatus km::VerifyUserPointer(const VerifyRules& rules, uintptr_t address, size_t size, const PageBuilder *pm) {
    //
    // Pointer must be aligned to the specified alignment.
    //
    if (!IsAlignedTo(address, rules.alignment)) {
        return OsStatusInvalidAddress;
    }

    //
    // If the size is not within our specified limits, it is invalid.
    //
    if (size < rules.minSize || size > rules.maxSize) {
        return OsStatusInvalidSpan;
    }

    //
    // If the size is not a multiple of the specified size, it is invalid.
    //
    if (size % rules.multiple != 0) {
        return OsStatusInvalidSpan;
    }

    //
    // If the range wraps around, it is always invalid.
    //
    uintptr_t begin = address;
    uintptr_t end = address;
    if (__builtin_add_overflow(address, size, &end)) {
        return OsStatusInvalidSpan;
    }

    //
    // If the address is not canonical, it is invalid.
    // TODO: will need to revisit this if I ever decide to enable LAM.
    //
    if (!pm->isCanonicalAddress(begin) || !pm->isCanonicalAddress(end)) {
        return OsStatusInvalidAddress;
    }

    //
    // Users should not have pointers into kernel memory.
    //
    if (pm->isHigherHalf(begin) || pm->isHigherHalf(end)) {
        return OsStatusInvalidAddress;
    }

    return OsStatusSuccess;
}

OsStatus km::VerifyUserRange(const VerifyRules& rules, uintptr_t begin, uintptr_t end, const PageBuilder *pm) {
    //
    // If the span is negative, it is always invalid.
    //
    if (begin > end) {
        return OsStatusInvalidSpan;
    }

    size_t size = end - begin;

    return VerifyUserPointer(rules, begin, size, pm);
}

OsStatus km::VerifyRangeMapped(uintptr_t begin, uintptr_t end, PageFlags flags, PageTables *pt) {
    for (uintptr_t addr = begin; addr < end; addr += x64::kPageSize) {
        PageFlags pf = pt->getMemoryFlags((void*)addr);
        if ((pf & flags) != flags) {
            return OsStatusInvalidData;
        }
    }

    return OsStatusSuccess;
}

OsStatus km::ReadUserRange(const VerifyRules& rules, uintptr_t begin, uintptr_t end, void *dst, const PageBuilder *pm, PageTables *pt) {
    //
    // First sanitize the input parameters.
    //
    if (OsStatus status = VerifyUserRange(rules, begin, end, pm)) {
        return status;
    }

    //
    // Then verify that they are mapped.
    //
    if (OsStatus status = VerifyRangeMapped(begin, end, PageFlags::eUser | PageFlags::eRead, pt)) {
        return status;
    }

    //
    // After we know the memory is safe to use we can copy the data.
    //
    size_t size = end - begin;
    memcpy(dst, (void*)begin, size);
    return OsStatusSuccess;
}
