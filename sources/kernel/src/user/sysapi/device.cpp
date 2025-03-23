#include "user/sysapi.hpp"

#include "process/device.hpp"
#include "process/thread.hpp"

#include "devices/stream.hpp"
#include "process/system.hpp"
#include "fs2/vfs.hpp"
#include "syscall.hpp"

// TODO: there should be a global registry of these, and they should be loaded from shared objects
static vfs2::INode *GetDefaultClass(sm::uuid uuid) {
    if (uuid == kOsStreamGuid) {
        return new dev::StreamDevice(1024);
    } else {
        return nullptr;
    }
}

class SystemInvokeContext final : public vfs2::IInvokeContext {
    km::CallContext *mContext;
    km::SystemObjects *mSystem;

    OsProcessHandle process() override {
        return mContext->process()->publicId();
    }

    OsThreadHandle thread() override {
        return mContext->thread()->publicId();
    }

    OsNodeHandle resolveNode(vfs2::INode *node) override {
        if (OsNodeHandle handle = mSystem->getNodeId(node)) {
            return handle;
        }

        km::Node *result = nullptr;
        if (mSystem->createNode(mContext->process(), node, &result) != OsStatusSuccess) {
            return OS_HANDLE_INVALID;
        }

        return result->publicId();
    }

public:
    SystemInvokeContext(km::CallContext *context, km::SystemObjects *system)
        : mContext(context)
        , mSystem(system)
    { }
};

OsCallResult um::DeviceOpen(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userCreateInfo = regs->arg0;

    OsDeviceCreateInfo createInfo{};
    if (OsStatus status = context->readObject(userCreateInfo, &createInfo)) {
        return km::CallError(status);
    }

    sm::uuid uuid = createInfo.InterfaceGuid;
    OsBuffer createData = createInfo.CreateData;
    OsBuffer openData = createInfo.OpenData;

    km::Process *process = nullptr;
    if (OsStatus status = SelectOwningProcess(system, context, createInfo.Process, &process)) {
        return km::CallError(status);
    }

    if (OsStatus status = VerifyBuffer(context, createData)) {
        return km::CallError(status);
    }

    if (OsStatus status = VerifyBuffer(context, openData)) {
        return km::CallError(status);
    }

    vfs2::VfsPath path;
    if (OsStatus status = ReadPath(context, createInfo.Path, &path)) {
        return km::CallError(status);
    }

    std::unique_ptr<vfs2::IHandle> handle = nullptr;
    OsStatus status = system->vfs->device(path, uuid, openData.Data, openData.Size, std::out_ptr(handle));

    if ((status == OsStatusNotFound) && (createInfo.Flags & eOsDeviceCreateNew)) {
        vfs2::INode *node = GetDefaultClass(uuid);
        if (node == nullptr) {
            return km::CallError(OsStatusNotFound);
        }

        status = system->vfs->mkdevice(path, node);
        if (status != OsStatusSuccess) {
            return km::CallError(status);
        }

        status = system->vfs->device(path, uuid, createData.Data, createData.Size, std::out_ptr(handle));
    }

    if (status != OsStatusSuccess) {
        return km::CallError(status);
    }

    km::Device *vnode = nullptr;
    if (OsStatus status = system->objects->addDevice(process, std::move(handle), &vnode)) {
        return km::CallError(status);
    }

    return km::CallOk(vnode->publicId());
}

OsCallResult um::DeviceClose(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userDevice = regs->arg0;
    km::Process *process = context->process();

    km::Device *node = system->objects->getDevice(km::DeviceId(OS_HANDLE_ID(userDevice)));
    if (node == nullptr) {
        return km::CallError(OsStatusInvalidHandle);
    }

    if (OsStatus status = system->objects->destroyDevice(process, node)) {
        return km::CallError(status);
    }

    return km::CallOk(0zu);
}

OsCallResult um::DeviceRead(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userHandle = regs->arg0;
    uint64_t userRequest = regs->arg1;

    OsDeviceReadRequest request{};
    if (OsStatus status = context->readObject(userRequest, &request)) {
        return km::CallError(status);
    }

    if (!context->isMapped((uint64_t)request.BufferFront, (uint64_t)request.BufferBack, km::PageFlags::eUser | km::PageFlags::eWrite)) {
        return km::CallError(OsStatusInvalidInput);
    }

    km::Device *node = system->objects->getDevice(km::DeviceId(OS_HANDLE_ID(userHandle)));
    if (node == nullptr) {
        return km::CallError(OsStatusInvalidHandle);
    }

    vfs2::ReadRequest readRequest {
        .begin = request.BufferFront,
        .end = request.BufferBack,
        .timeout = request.Timeout,
    };
    vfs2::ReadResult result{};
    if (OsStatus status = node->handle->read(readRequest, &result)) {
        return km::CallError(status);
    }

    return km::CallOk(result.read);
}

OsCallResult um::DeviceWrite(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userHandle = regs->arg0;
    uint64_t userRequest = regs->arg1;

    OsDeviceReadRequest request{};
    if (OsStatus status = context->readObject(userRequest, &request)) {
        return km::CallError(status);
    }

    if (!context->isMapped((uint64_t)request.BufferFront, (uint64_t)request.BufferBack, km::PageFlags::eUser | km::PageFlags::eRead)) {
        return km::CallError(OsStatusInvalidInput);
    }

    km::Device *node = system->objects->getDevice(km::DeviceId(OS_HANDLE_ID(userHandle)));
    if (node == nullptr) {
        return km::CallError(OsStatusInvalidHandle);
    }

    vfs2::WriteRequest writeRequest {
        .begin = request.BufferFront,
        .end = request.BufferBack,
        .timeout = request.Timeout,
    };
    vfs2::WriteResult result{};
    if (OsStatus status = node->handle->write(writeRequest, &result)) {
        return km::CallError(status);
    }

    return km::CallOk(result.write);
}

OsCallResult um::DeviceInvoke(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userHandle = regs->arg0;
    uint64_t userFunction = regs->arg1;
    uint64_t userData = regs->arg2;
    uint64_t userSize = regs->arg3;

    if (!context->isMapped((uint64_t)userData, (uint64_t)userData + userSize, km::PageFlags::eUserData)) {
        return km::CallError(OsStatusInvalidInput);
    }

    km::Device *node = system->objects->getDevice(km::DeviceId(OS_HANDLE_ID(userHandle)));
    if (node == nullptr) {
        return km::CallError(OsStatusInvalidHandle);
    }

    SystemInvokeContext invoke { context, system->objects };

    if (OsStatus status = node->handle->invoke(&invoke, userFunction, (void*)userData, userSize)) {
        return km::CallError(status);
    }

    return km::CallOk(0zu);
}

OsCallResult um::DeviceStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userHandle = regs->arg0;
    uint64_t userStat = regs->arg1;

    km::Device *device = system->objects->getDevice(km::DeviceId(OS_HANDLE_ID(userHandle)));
    if (device == nullptr) {
        return km::CallError(OsStatusInvalidHandle);
    }

    vfs2::HandleInfo handleInfo = device->handle->info();
    vfs2::NodeInfo nodeInfo = handleInfo.node->info();

    OsDeviceInfo result{};
    size_t len = std::min(sizeof(result.Name), nodeInfo.name.count());
    std::memcpy(result.Name, nodeInfo.name.data(), len);

    result.InterfaceGuid = handleInfo.guid;
    result.Node = system->objects->getNodeId(handleInfo.node);

    // TODO: fill in the rest of the fields

    if (OsStatus status = context->writeObject(userStat, result)) {
        return km::CallError(status);
    }

    return km::CallOk(0zu);
}
