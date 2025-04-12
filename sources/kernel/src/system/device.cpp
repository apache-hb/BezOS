#include "system/device.hpp"
#include "system/system.hpp"
#include "system/node.hpp"
#include "system/process.hpp"

#include "fs2/vfs.hpp"

static sys2::ObjectName GetHandleName(vfs2::IHandle *handle) {
    auto hInfo = handle->info();
    auto nInfo = hInfo.node->info();
    return nInfo.name;
}

sys2::Device::Device(std::unique_ptr<vfs2::IHandle> handle)
    : BaseObject(GetHandleName(handle.get()))
    , mVfsHandle(std::move(handle))
{ }

sys2::DeviceHandle::DeviceHandle(sm::RcuSharedPtr<Device> device, OsHandle handle, DeviceAccess access)
    : BaseHandle(device, handle, access)
{ }

OsStatus sys2::SysDeviceOpen(InvokeContext *context, DeviceOpenInfo info, DeviceHandle **handle) {
    std::unique_ptr<vfs2::IHandle> vfsHandle;
    ProcessHandle *hProcess = context->process;
    vfs2::VfsRoot *vfs = context->system->mVfsRoot;

    if (!hProcess->hasAccess(ProcessAccess::eIoControl)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Process> process = hProcess->getProcess();
    if (OsStatus status = vfs->device(info.path, info.uuid, info.data, info.size, std::out_ptr(vfsHandle))) {
        return status;
    }

    sm::RcuSharedPtr<Device> device = sm::rcuMakeShared<Device>(&context->system->rcuDomain(), std::move(vfsHandle));
    if (!device) {
        return OsStatusOutOfMemory;
    }

    DeviceHandle *result = new (std::nothrow) DeviceHandle(device, process->newHandleId(eOsHandleDevice), DeviceAccess::eAll);
    if (!result) {
        return OsStatusOutOfMemory;
    }

    process->addHandle(result);
    *handle = result;

    return OsStatusSuccess;
}

OsStatus sys2::SysDeviceClose(InvokeContext *context, DeviceHandle *handle) {
    ProcessHandle *parent = context->process;

    if (!handle->hasAccess(DeviceAccess::eDestroy)) {
        return OsStatusAccessDenied;
    }

    if (!parent->hasAccess(ProcessAccess::eIoControl)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Device> device = handle->getDevice();
    sm::RcuSharedPtr<Process> process = parent->getProcess();

    if (OsStatus status = process->removeHandle(handle)) {
        return status;
    }

    delete handle;

    return OsStatusSuccess;
}

OsStatus sys2::SysDeviceRead(InvokeContext *, DeviceReadInfo info, DeviceReadResult *result) {
    DeviceHandle *handle = info.device;
    if (!handle->hasAccess(DeviceAccess::eRead)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Device> device = handle->getDevice();
    vfs2::IHandle *vfsHandle = device->getVfsHandle();
    vfs2::ReadRequest vfsRequest {
        .begin = info.front,
        .end = info.back,
        .offset = info.offset,
        .timeout = info.timeout,
    };
    vfs2::ReadResult vfsResult{};

    if (OsStatus status = vfsHandle->read(vfsRequest, &vfsResult)) {
        return status;
    }

    *result = DeviceReadResult {
        .read = vfsResult.read,
    };

    return OsStatusSuccess;
}

OsStatus sys2::SysDeviceWrite(InvokeContext *, DeviceWriteInfo info, DeviceWriteResult *result) {
    DeviceHandle *handle = info.device;
    if (!handle->hasAccess(DeviceAccess::eWrite)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Device> device = handle->getDevice();
    vfs2::IHandle *vfsHandle = device->getVfsHandle();
    vfs2::WriteRequest vfsRequest {
        .begin = info.front,
        .end = info.back,
        .offset = info.offset,
        .timeout = info.timeout,
    };
    vfs2::WriteResult vfsResult{};

    if (OsStatus status = vfsHandle->write(vfsRequest, &vfsResult)) {
        return status;
    }

    *result = DeviceWriteResult {
        .write = vfsResult.write,
    };

    return OsStatusSuccess;
}

#if 0
OsStatus sys2::SysDeviceInvoke(InvokeContext *context, DeviceInvokeInfo info) {
    DeviceHandle *handle = info.device;
    if (!handle->hasAccess(DeviceAccess::eInvoke)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Device> device = handle->getDevice();
    vfs2::IHandle *vfsHandle = device->getVfsHandle();

    if (OsStatus status = vfsHandle->invoke(context, info.method, info.data, info.size)) {
        return status;
    }

    return OsStatusSuccess;
}
#endif

OsStatus sys2::SysDeviceStat(InvokeContext *, DeviceHandle *handle, DeviceStat *result) {
    if (!handle->hasAccess(DeviceAccess::eStat)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Device> device = handle->getDevice();
    vfs2::IHandle *vfsHandle = device->getVfsHandle();

    auto hInfo = vfsHandle->info();
    auto nInfo = hInfo.node->info();

    *result = DeviceStat {
        .name = nInfo.name,
    };

    return OsStatusSuccess;
}
