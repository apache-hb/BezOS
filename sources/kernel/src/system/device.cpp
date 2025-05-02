#include "system/device.hpp"
#include "devices/stream.hpp"
#include "system/system.hpp"
#include "system/node.hpp"
#include "system/process.hpp"

#include "fs2/vfs.hpp"

class SysInvokeContext final : public vfs2::IInvokeContext {
    sys::InvokeContext *mContext;

public:
    SysInvokeContext(sys::InvokeContext *context)
        : mContext(context)
    { }

    OsNodeHandle resolveNode(sm::RcuSharedPtr<vfs2::INode> node) override {
        OsNodeHandle result = OS_HANDLE_INVALID;

        if (OsStatus _ = sys::SysNodeCreate(mContext, node, &result)) {
            return OS_HANDLE_INVALID;
        }

        return result;
    }
};

static sys::ObjectName GetHandleName(vfs2::IHandle *handle) {
    auto hInfo = handle->info();
    auto nInfo = hInfo.node->info();
    return nInfo.name;
}

sys::Device::Device(std::unique_ptr<vfs2::IHandle> handle)
    : BaseObject(GetHandleName(handle.get()))
    , mVfsHandle(std::move(handle))
{ }

sys::DeviceHandle::DeviceHandle(sm::RcuSharedPtr<Device> device, OsHandle handle, DeviceAccess access)
    : BaseHandle(device, handle, access)
{ }

static OsStatus DeviceOpenExisting(sys::InvokeContext *context, sys::DeviceOpenInfo info, OsDeviceHandle *outHandle) {
    vfs2::VfsRoot *vfs = context->system->mVfsRoot;
    std::unique_ptr<vfs2::IHandle> vfsHandle;

    if (OsStatus status = vfs->device(info.path, info.interface, info.data, info.size, std::out_ptr(vfsHandle))) {
        return status;
    }

    sm::RcuSharedPtr<sys::Device> device = sm::rcuMakeShared<sys::Device>(&context->system->rcuDomain(), std::move(vfsHandle));
    if (!device) {
        return OsStatusOutOfMemory;
    }

    sys::DeviceHandle *result = new (std::nothrow) sys::DeviceHandle(device, context->process->newHandleId(eOsHandleDevice), sys::DeviceAccess::eAll);
    if (!result) {
        return OsStatusOutOfMemory;
    }

    context->process->addHandle(result);
    *outHandle = result->getHandle();

    return OsStatusSuccess;
}

static OsStatus DeviceCreateNew(sys::InvokeContext *context, sys::DeviceOpenInfo info, OsDeviceHandle *outHandle) {
    vfs2::VfsRoot *vfs = context->system->mVfsRoot;
    sm::RcuSharedPtr<vfs2::INode> vfsNode;
    std::unique_ptr<vfs2::IHandle> vfsHandle;
    OsStatus status = OsStatusSuccess;

    if (info.interface == kOsFileGuid) {
        status = vfs->create(info.path, &vfsNode);
    } else if (info.interface == kOsFolderGuid) {
        status = vfs->mkdir(info.path, &vfsNode);
    } else if (info.interface == kOsStreamGuid) {
        vfsNode = sm::rcuMakeShared<dev::StreamDevice>(vfs->domain(), 1024);
        if (!vfsNode) {
            return OsStatusOutOfMemory;
        }

        status = vfs->mkdevice(info.path, vfsNode);
    } else {
        KmDebugMessage("[VFS] Failed to create device '", info.path, "'::", info.interface, "\n");
        return OsStatusInvalidType;
    }

    if (status != OsStatusSuccess) {
        return status;
    }

    if (vfsNode == nullptr) {
        KmDebugMessage("[VFS] Internal failure when creating device '", info.path, "'::", info.interface, "\n");
        return OsStatusOutOfMemory;
    }

    if (OsStatus status = vfsNode->query(info.interface, info.data, info.size, std::out_ptr(vfsHandle))) {
        return status;
    }

    sm::RcuSharedPtr<sys::Device> device = sm::rcuMakeShared<sys::Device>(&context->system->rcuDomain(), std::move(vfsHandle));
    if (!device) {
        return OsStatusOutOfMemory;
    }

    sys::DeviceHandle *result = new (std::nothrow) sys::DeviceHandle(device, context->process->newHandleId(eOsHandleDevice), sys::DeviceAccess::eAll);
    if (!result) {
        return OsStatusOutOfMemory;
    }

    context->process->addHandle(result);
    *outHandle = result->getHandle();

    return OsStatusSuccess;
}

static OsStatus DeviceOpenAlways(sys::InvokeContext *context, sys::DeviceOpenInfo info, OsDeviceHandle *outHandle) {
    if (OsStatus status = DeviceOpenExisting(context, info, outHandle)) {
        if (status != OsStatusNotFound) {
            return status;
        }

        if (OsStatus status = DeviceCreateNew(context, info, outHandle)) {
            return status;
        }
    }

    return OsStatusSuccess;
}

OsStatus sys::SysDeviceOpen(InvokeContext *context, DeviceOpenInfo info, OsDeviceHandle *outHandle) {
    switch (info.flags) {
    case eOsDeviceOpenExisting:
        return DeviceOpenExisting(context, info, outHandle);
    case eOsDeviceCreateNew:
        return DeviceCreateNew(context, info, outHandle);
    case eOsDeviceOpenAlways:
        return DeviceOpenAlways(context, info, outHandle);

    default:
        return OsStatusInvalidInput;
    }
}

OsStatus sys::SysDeviceClose(InvokeContext *context, OsDeviceHandle handle) {
    DeviceHandle *hDevice = nullptr;
    if (OsStatus status = context->process->findHandle(handle, &hDevice)) {
        return status;
    }

    if (!hDevice->hasAccess(DeviceAccess::eDestroy)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Device> device = hDevice->getDevice();

    if (OsStatus status = context->process->removeHandle(hDevice)) {
        return status;
    }

    return OsStatusSuccess;
}

OsStatus sys::SysDeviceRead(InvokeContext *context, OsDeviceHandle handle, OsDeviceReadRequest request, OsSize *outRead) {
    DeviceHandle *hDevice = nullptr;
    if (OsStatus status = context->process->findHandle(handle, &hDevice)) {
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

OsStatus sys::SysDeviceWrite(InvokeContext *context, OsDeviceHandle handle, OsDeviceWriteRequest request, OsSize *outWrite) {
    DeviceHandle *hDevice = nullptr;
    if (OsStatus status = context->process->findHandle(handle, &hDevice)) {
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

OsStatus sys::SysDeviceInvoke(InvokeContext *context, OsDeviceHandle handle, uint64_t function, void *data, size_t size) {
    DeviceHandle *hDevice = nullptr;
    if (OsStatus status = context->process->findHandle(handle, &hDevice)) {
        return status;
    }

    if (!hDevice->hasAccess(DeviceAccess::eInvoke)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Device> device = hDevice->getDevice();
    vfs2::IHandle *vfsHandle = device->getVfsHandle();

    SysInvokeContext invoke { context };
    if (OsStatus status = vfsHandle->invoke(&invoke, function, data, size)) {
        return status;
    }

    return OsStatusSuccess;
}

OsStatus sys::SysDeviceStat(InvokeContext *context, OsDeviceHandle handle, OsDeviceInfo *info) {
    DeviceHandle *hDevice = nullptr;
    if (OsStatus status = context->process->findHandle(handle, &hDevice)) {
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
