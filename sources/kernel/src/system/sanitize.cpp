#include "system/sanitize.hpp"
#include "system/device.hpp"
#include "system/process.hpp"

OsStatus sys::Sanitize<sys::VmemCreateInfo>::sanitize(InvokeContext *context, const OsVmemCreateInfo *info, VmemCreateInfo *result) noexcept {
    // input components
    size_t size = info->Size;
    size_t align = info->Alignment;
    sm::VirtualAddress base = info->BaseAddress;
    OsMemoryAccess access = info->Access;
    OsProcessHandle handle = info->Process;

    // output components
    km::PageFlags flags = km::PageFlags::eUser;
    bool commit = false;
    bool reserve = false;
    bool zeroMemory = false;
    bool addressIsHint = false;
    sm::RcuSharedPtr<Process> process;

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

    if (!commit && !reserve) {
        return OsStatusInvalidInput;
    }

    if (access != 0) {
        return OsStatusInvalidInput;
    }

    if (handle == OS_HANDLE_INVALID) {
        process = context->process;
    } else {
        ProcessHandle *hProcess = nullptr;
        if (OsStatus status = SysFindHandle(context, handle, &hProcess)) {
            return status;
        }

        process = hProcess->getProcess();
    }

    VmemCreateMode mode = commit ? VmemCreateMode::eCommit : VmemCreateMode::eReserve;

    *result = VmemCreateInfo {
        .size = size,
        .alignment = align,
        .baseAddress = base,
        .flags = flags,
        .zeroMemory = zeroMemory,
        .mode = mode,
        .addressIsHint = addressIsHint,
        .process = process,
    };

    return OsStatusSuccess;
}

OsStatus sys::Sanitize<sys::VmemMapInfo>::sanitize(InvokeContext *context, const OsVmemMapInfo *info [[gnu::nonnull]], VmemMapInfo *result [[gnu::nonnull]]) noexcept {
    size_t size = info->Size;
    sm::VirtualAddress base = info->DstAddress;
    sm::VirtualAddress src = info->SrcAddress;
    OsMemoryAccess access = info->Access;
    OsHandle srcHandle = info->Source;
    OsProcessHandle dstHandle = info->Process;

    km::PageFlags flags = km::PageFlags::eUser;
    bool addressIsHint = false;
    sm::RcuSharedPtr<IObject> srcObject;
    sm::RcuSharedPtr<Process> dstProcess;

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

    if (size == 0) {
        return OsStatusInvalidInput;
    }

    if (!src.isAlignedTo(x64::kPageSize)) {
        return OsStatusInvalidInput;
    }

    if (takeFlag(eOsMemoryAddressHint)) {
        addressIsHint = true;
    }

    if (addressIsHint && base.isNull()) {
        return OsStatusInvalidInput;
    }

    if (!base.isAlignedTo(x64::kPageSize) && !base.isNull()) {
        return OsStatusInvalidInput;
    }

    applyFlag(eOsMemoryRead, km::PageFlags::eRead);
    applyFlag(eOsMemoryWrite, km::PageFlags::eWrite);
    applyFlag(eOsMemoryExecute, km::PageFlags::eExecute);

    if (access != 0) {
        return OsStatusInvalidInput;
    }

    if (srcHandle == OS_HANDLE_INVALID) {
        return OsStatusInvalidHandle;
    }

    if (OS_HANDLE_TYPE(srcHandle) == eOsHandleProcess) {
        ProcessHandle *hSource = nullptr;
        if (OsStatus status = SysFindHandle(context, srcHandle, &hSource)) {
            return status;
        }

        srcObject = hSource->getProcess();
    } else if (OS_HANDLE_TYPE(srcHandle) == eOsHandleDevice) {
        DeviceHandle *hDevice = nullptr;
        if (OsStatus status = SysFindHandle(context, srcHandle, &hDevice)) {
            return status;
        }

        sm::RcuSharedPtr device = hDevice->getDevice();
        vfs2::HandleInfo info = device->getVfsHandle()->info();
        if (info.guid != sm::uuid(kOsFileGuid)) {
            return OsStatusInvalidHandle;
        }

        srcObject = device;
    } else {
        return OsStatusInvalidHandle;
    }

    if (dstHandle == OS_HANDLE_INVALID) {
        dstProcess = context->process;
    } else {
        ProcessHandle *hProcess = nullptr;
        if (OsStatus status = SysFindHandle(context, dstHandle, &hProcess)) {
            return status;
        }

        dstProcess = hProcess->getProcess();
    }

    *result = VmemMapInfo {
        .size = size,
        .baseAddress = base,
        .flags = flags,
        .srcAddress = src,
        .srcObject = srcObject,
        .process = dstProcess,
    };

    return OsStatusSuccess;
}
