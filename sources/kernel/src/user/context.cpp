#include "user/context.hpp"

#include "memory/paging.hpp"
#include "memory/page_tables.hpp"

#include <bezos/facility/process.h>

static constexpr bool isAlignedTo(uint64_t address, size_t alignment) {
    return (address & (alignment - 1)) == 0;
}

OsStatus km::VerifyUserPointer(const VerifyRules& rules, uintptr_t address, size_t size, const PageBuilder *pm) {
    //
    // Pointer must be aligned to the specified alignment.
    //
    if (!isAlignedTo(address, rules.alignment)) {
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
    //
    // Walk the range and test each page.
    // TODO: this could be more efficiently implemented inside PageTables as a single call.
    //
    for (uintptr_t addr = begin; addr < end; addr += x64::kPageSize) {
        PageFlags pf = pt->getMemoryFlags((void*)addr);
        if ((pf & flags) != flags) {
            return OsStatusInvalidData;
        }
    }

    return OsStatusSuccess;
}

OsStatus km::VerifyRangeValid(const VerifyRules& rules, uintptr_t begin, uintptr_t end, PageFlags flags, const PageBuilder *pm, PageTables *pt) {
    //
    // First sanitize the input parameters.
    //
    if (OsStatus status = VerifyUserRange(rules, begin, end, pm)) {
        return status;
    }

    //
    // Then verify that they are mapped.
    //
    if (OsStatus status = VerifyRangeMapped(begin, end, flags, pt)) {
        return status;
    }

    //
    // If the memory is mapped and the range is valid then all is good.
    //
    return OsStatusSuccess;
}

OsStatus km::ReadUserRange(const VerifyRules& rules, uintptr_t begin, uintptr_t end, void *dst, const PageBuilder *pm, PageTables *pt) {
    if (OsStatus status = VerifyRangeValid(rules, begin, end, PageFlags::eUser | PageFlags::eRead, pm, pt)) {
        return status;
    }

    size_t size = end - begin;
    memcpy(dst, (void*)begin, size);
    return OsStatusSuccess;
}

OsStatus km::WriteUserRange(const VerifyRules& rules, uintptr_t begin, uintptr_t end, const void *src, const PageBuilder *pm, PageTables *pt) {
    if (OsStatus status = VerifyRangeValid(rules, begin, end, PageFlags::eUser | PageFlags::eWrite, pm, pt)) {
        return status;
    }

    size_t size = end - begin;
    memcpy((void*)begin, src, size);
    return OsStatusSuccess;
}

OsStatus km::ReadUserArray(const VerifyRules& rules, uintptr_t begin, uintptr_t end, stdx::Vector2<std::byte>& dst, const PageBuilder *pm, PageTables *pt) {
    if (OsStatus status = VerifyRangeValid(rules, begin, end, PageFlags::eUser | PageFlags::eRead, pm, pt)) {
        return status;
    }

    size_t size = end - begin;
    dst.resize(size);
    memcpy(dst.data(), (void*)begin, size);
    return OsStatusSuccess;
}

OsStatus km::ReadUserObject(const VerifyRules& rules, uintptr_t address, size_t size, void *dst, const PageBuilder *pm, PageTables *pt) {
    uintptr_t end;
    if (__builtin_add_overflow(address, size, &end)) {
        return OsStatusInvalidSpan;
    }

    return ReadUserRange(rules, address, end, dst, pm, pt);
}

OsStatus km::WriteUserObject(const VerifyRules& rules, uintptr_t address, size_t size, const void *src, const PageBuilder *pm, PageTables *pt) {
    uintptr_t end;
    if (__builtin_add_overflow(address, size, &end)) {
        return OsStatusInvalidSpan;
    }

    return WriteUserRange(rules, address, end, src, pm, pt);
}

static OsStatus VerifySingleArg(std::span<const std::byte> &args) {
    //
    // If theres not enough data for the header, it is invalid.
    //
    if (args.size_bytes() < sizeof(OsProcessParam)) {
        return OsStatusInvalidSpan;
    }

    //
    // If the data size is larger than the remaining data, it is invalid.
    //
    OsProcessParam *param = (OsProcessParam*)args.data();
    if (param->DataSize > (args.size_bytes() - sizeof(OsProcessParam))) {
        return OsStatusInvalidSpan;
    }

    args = args.subspan(sizeof(OsProcessParam) + param->DataSize);
    return OsStatusSuccess;
}

OsStatus km::VerifyProcessCreateArgs(std::span<const std::byte> args) {
    while (!args.empty()) {
        if (OsStatus status = VerifySingleArg(args)) {
            return status;
        }
    }

    return OsStatusSuccess;
}
