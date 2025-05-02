#include "system/sanitize.hpp"

OsStatus sys::Sanitize<sys::VmemCreateInfo>::sanitize(const OsVmemCreateInfo *info, sys::VmemCreateInfo *result) noexcept {
    // input components
    size_t size = info->Size;
    size_t align = info->Alignment;
    sm::VirtualAddress base = info->BaseAddress;
    OsMemoryAccess access = info->Access;

    // output components
    km::PageFlags flags = km::PageFlags::eUser;
    bool commit = false;
    bool reserve = false;
    bool zeroMemory = false;
    bool addressIsHint = false;

    auto takeFlag = [&](OsMemoryAccess test) -> bool {
        if ((access & test) == test) {
            access &= ~test;
            return true;
        }

        return false;
    };

    auto applyFlag = [&](OsMemoryAccess test, km::PageFlags flag) {
        if (takeFlag(test)) {
            flags |= flag;
        }
    };

    if (takeFlag(eOsMemoryAddressHint)) {
        addressIsHint = true;
    }

    if (size == 0 || size % x64::kPageSize != 0) {
        return OsStatusInvalidInput;
    }

    if (align == 0) {
        align = x64::kPageSize;
    }

    if (align % x64::kPageSize != 0) {
        return OsStatusInvalidInput;
    }

    if (!base.isNull() && !base.isAlignedTo(align)) {
        return OsStatusInvalidInput;
    }

    applyFlag(eOsMemoryRead, km::PageFlags::eRead);
    applyFlag(eOsMemoryWrite, km::PageFlags::eWrite);
    applyFlag(eOsMemoryExecute, km::PageFlags::eExecute);

    if (takeFlag(eOsMemoryReserve)) {
        reserve = true;
    }

    if (takeFlag(eOsMemoryCommit)) {
        commit = true;
    }

    if (takeFlag(eOsMemoryDiscard)) {
        zeroMemory = true;
    }

    if (access != 0) {
        return OsStatusInvalidInput;
    }

    *result = VmemCreateInfo {
        .size = size,
        .alignment = align,
        .baseAddress = base,
        .flags = flags,
        .zeroMemory = zeroMemory,
        .commit = commit,
        .reserve = reserve,
        .addressIsHint = addressIsHint,
    };
    return OsStatusSuccess;
}

OsStatus sys::Sanitize<sys::VmemMapInfo>::sanitize(const ApiObject *info [[gnu::nonnull]], KernelObject *result [[gnu::nonnull]]) noexcept {
    size_t size = info->Size;
    sm::VirtualAddress base = info->DstAddress;
    sm::VirtualAddress src = info->SrcAddress;
    OsMemoryAccess access = info->Access;
    OsHandle srcHandle = info->Source;
    OsHandle dstHandle = info->Process;


}
