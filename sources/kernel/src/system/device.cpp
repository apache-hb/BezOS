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

OsStatus sys2::SysDeviceOpen(InvokeContext *context, DeviceOpenInfo info, OsDeviceHandle *outHandle) {
    std::unique_ptr<vfs2::IHandle> vfsHandle;
    ProcessHandle *hProcess = context->process;
    vfs2::VfsRoot *vfs = context->system->mVfsRoot;

    if (!hProcess->hasAccess(ProcessAccess::eIoControl)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Process> process = hProcess->getProcess();
    if (OsStatus status = vfs->device(info.path, info.interface, nullptr, 0, std::out_ptr(vfsHandle))) {
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
    *outHandle = result->getHandle();

    return OsStatusSuccess;
}

OsStatus sys2::SysDeviceClose(InvokeContext *context, OsDeviceHandle handle) {
    ProcessHandle *parent = context->process;

    if (!parent->hasAccess(ProcessAccess::eIoControl)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Process> process = parent->getProcess();

    DeviceHandle *hDevice = nullptr;
    if (OsStatus status = process->findHandle(handle, &hDevice)) {
        return status;
    }

    if (!hDevice->hasAccess(DeviceAccess::eDestroy)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Device> device = hDevice->getDevice();

    if (OsStatus status = process->removeHandle(hDevice)) {
        return status;
    }

    return OsStatusSuccess;
}

OsStatus sys2::SysDeviceRead(InvokeContext *context, OsDeviceHandle handle, OsDeviceReadRequest request, OsSize *outRead) {
    ProcessHandle *parent = context->process;

    if (!parent->hasAccess(ProcessAccess::eIoControl)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Process> process = parent->getProcess();

    DeviceHandle *hDevice = nullptr;
    if (OsStatus status = process->findHandle(handle, &hDevice)) {
        return status;
    }

    if (!hDevice->hasAccess(DeviceAccess::eRead)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Device> device = hDevice->getDevice();
    vfs2::IHandle *vfsHandle = device->getVfsHandle();
    vfs2::ReadRequest vfsRequest {
        .begin = request.BufferFront,
        .end = request.BufferBack,
        .offset = request.Offset,
        .timeout = request.Timeout,
    };
    vfs2::ReadResult vfsResult{};

    if (OsStatus status = vfsHandle->read(vfsRequest, &vfsResult)) {
        return status;
    }

    *outRead = vfsResult.read;

    return OsStatusSuccess;
}

OsStatus sys2::SysDeviceWrite(InvokeContext *context, OsDeviceHandle handle, OsDeviceWriteRequest request, OsSize *outWrite) {
    ProcessHandle *parent = context->process;

    if (!parent->hasAccess(ProcessAccess::eIoControl)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Process> process = parent->getProcess();

    DeviceHandle *hDevice = nullptr;
    if (OsStatus status = process->findHandle(handle, &hDevice)) {
        return status;
    }

    if (!hDevice->hasAccess(DeviceAccess::eWrite)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Device> device = hDevice->getDevice();
    vfs2::IHandle *vfsHandle = device->getVfsHandle();
    vfs2::WriteRequest vfsRequest {
        .begin = request.BufferFront,
        .end = request.BufferBack,
        .offset = request.Offset,
        .timeout = request.Timeout,
    };
    vfs2::WriteResult vfsResult{};

    if (OsStatus status = vfsHandle->write(vfsRequest, &vfsResult)) {
        return status;
    }

    *outWrite = vfsResult.write;

    return OsStatusSuccess;
}

#if 0
OsStatus sys2::SysDeviceInvoke(InvokeContext *context, OsDeviceHandle handle, uint64_t function, void *data, size_t size) {
    ProcessHandle *parent = context->process;

    if (!parent->hasAccess(ProcessAccess::eIoControl)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Process> process = parent->getProcess();

    DeviceHandle *hDevice = nullptr;
    if (OsStatus status = process->findHandle(handle, &hDevice)) {
        return status;
    }

    if (!hDevice->hasAccess(DeviceAccess::eInvoke)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Device> device = hDevice->getDevice();
    vfs2::IHandle *vfsHandle = device->getVfsHandle();

    if (OsStatus status = vfsHandle->invoke(context, function, data, size)) {
        return status;
    }

    return OsStatusSuccess;
}
#endif

OsStatus sys2::SysDeviceStat(InvokeContext *context, OsDeviceHandle handle, OsDeviceInfo *info) {
    ProcessHandle *parent = context->process;

    if (!parent->hasAccess(ProcessAccess::eIoControl)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Process> process = parent->getProcess();

    DeviceHandle *hDevice = nullptr;
    if (OsStatus status = process->findHandle(handle, &hDevice)) {
        return status;
    }

    if (!hDevice->hasAccess(DeviceAccess::eStat)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Device> device = hDevice->getDevice();
    vfs2::IHandle *vfsHandle = device->getVfsHandle();

    auto hInfo = vfsHandle->info();
    auto nInfo = hInfo.node->info();

    OsDeviceInfo result {
        .InterfaceGuid = hInfo.guid,
    };
    size_t size = std::min(sizeof(result.Name), nInfo.name.count());
    std::memcpy(result.Name, nInfo.name.data(), size);

    *info = result;

    return OsStatusSuccess;
}
