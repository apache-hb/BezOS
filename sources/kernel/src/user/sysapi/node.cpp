#include "log.hpp"
#include "system/create.hpp"
#include "system/schedule.hpp"
#include "system/system.hpp"
#include "user/sysapi.hpp"

#include "process/device.hpp"

#include "process/system.hpp"
#include "fs2/vfs.hpp"
#include "syscall.hpp"

// TODO: make this runtime configurable
static constexpr size_t kMaxPathSize = 0x1000;

OsStatus um::ReadPath(km::CallContext *context, OsPath user, vfs2::VfsPath *path) {
    vfs2::VfsString text;
    if (OsStatus status = context->readString((uint64_t)user.Front, (uint64_t)user.Back, kMaxPathSize, &text)) {
        return status;
    }

    if (!vfs2::VerifyPathText(text)) {
        KmDebugMessage("[PROC] Invalid path: '", text, "'\n");
        return OsStatusInvalidPath;
    }

    *path = vfs2::VfsPath{std::move(text)};
    return OsStatusSuccess;
}

OsStatus um::VerifyBuffer(km::CallContext *context, OsBuffer buffer) {
    if ((buffer.Data == nullptr) ^ (buffer.Size == 0)) {
        return OsStatusInvalidInput;
    }

    if (buffer.Size != 0) {
        if (!context->isMapped((uint64_t)buffer.Data, (uint64_t)buffer.Data + buffer.Size, km::PageFlags::eUser | km::PageFlags::eRead)) {
            return OsStatusInvalidInput;
        }
    }

    return OsStatusSuccess;
}

OsStatus um::SelectOwningProcess(km::System *system, km::CallContext *context, OsProcessHandle handle, km::Process **result) {
    km::Process *process = nullptr;
    if (handle != OS_HANDLE_INVALID) {
        process = system->objects->getProcess(km::ProcessId(OS_HANDLE_ID(handle)));
    } else {
        process = context->process();
    }

    if (process == nullptr) {
        return OsStatusInvalidHandle;
    }

    *result = process;
    return OsStatusSuccess;
}

static OsCallResult UserNodeOpen(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userCreateInfo = regs->arg0;

    km::Process *process = nullptr;
    sm::RcuSharedPtr<vfs2::INode> node = nullptr;
    km::Node *result = nullptr;
    vfs2::VfsPath path;
    OsNodeCreateInfo createInfo{};

    if (OsStatus status = context->readObject(userCreateInfo, &createInfo)) {
        return km::CallError(status);
    }

    if (OsStatus status = um::SelectOwningProcess(system, context, createInfo.Process, &process)) {
        return km::CallError(status);
    }

    if (OsStatus status = um::ReadPath(context, createInfo.Path, &path)) {
        return km::CallError(status);
    }

    if (OsStatus status = system->vfs->lookup(path, &node)) {
        return km::CallError(status);
    }

    if (OsNodeHandle existing = system->objects->getNodeId(node)) {
        return km::CallOk(existing);
    }

    if (OsStatus status = system->objects->createNode(process, node, &result)) {
        return km::CallError(status);
    }

    return km::CallOk(result->publicId());
}

static OsCallResult UserNodeClose(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userHandle = regs->arg0;

    km::Node *node = system->objects->getNode(km::NodeId(OS_HANDLE_ID(userHandle)));
    if (node == nullptr) {
        return km::CallError(OsStatusInvalidHandle);
    }

    if (OsStatus status = system->objects->destroyNode(context->process(), node)) {
        return km::CallError(status);
    }

    return km::CallOk(0zu);
}

static OsCallResult UserNodeQuery(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userHandle = regs->arg0;
    uint64_t userQuery = regs->arg1;

    km::Node *node = system->objects->getNode(km::NodeId(OS_HANDLE_ID(userHandle)));
    if (node == nullptr) {
        return km::CallError(OsStatusInvalidHandle);
    }

    vfs2::NodeInfo info = node->node->info();

    OsNodeInfo result{};
    size_t len = std::min(sizeof(result.Name), info.name.count());
    std::memcpy(result.Name, info.name.data(), len);

    if (OsStatus status = context->writeObject(userQuery, result)) {
        return km::CallError(status);
    }

    return km::CallOk(0zu);
}

static OsCallResult UserNodeStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userHandle = regs->arg0;
    uint64_t userStat = regs->arg1;

    km::Node *node = system->objects->getNode(km::NodeId(OS_HANDLE_ID(userHandle)));
    if (node == nullptr) {
        return km::CallError(OsStatusInvalidHandle);
    }

    vfs2::NodeInfo info = node->node->info();

    OsNodeInfo result{};
    size_t len = std::min(sizeof(result.Name), info.name.count());
    std::memcpy(result.Name, info.name.data(), len);

    if (OsStatus status = context->writeObject(userStat, result)) {
        return km::CallError(status);
    }

    return km::CallOk(0zu);
}

static OsCallResult NewNodeOpen(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userCreateInfo = regs->arg0;

    vfs2::VfsPath path;
    OsNodeCreateInfo createInfo{};

    if (OsStatus status = context->readObject(userCreateInfo, &createInfo)) {
        return km::CallError(status);
    }

    if (OsStatus status = um::ReadPath(context, createInfo.Path, &path)) {
        return km::CallError(status);
    }

    sys2::InvokeContext invoke { system->sys, sys2::GetCurrentProcess() };
    sys2::NodeOpenInfo openInfo {
        .path = path,
    };

    OsNodeHandle node = OS_HANDLE_INVALID;

    if (OsStatus status = sys2::SysNodeOpen(&invoke, openInfo, &node)) {
        return km::CallError(status);
    }

    return km::CallOk(node);
}

static OsCallResult NewNodeClose(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userHandle = regs->arg0;

    sys2::InvokeContext invoke { system->sys, sys2::GetCurrentProcess() };
    if (OsStatus status = sys2::SysNodeClose(&invoke, userHandle)) {
        return km::CallError(status);
    }

    return km::CallOk(0zu);
}

static OsCallResult NewNodeQuery(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userHandle = regs->arg0;
    uint64_t userQuery = regs->arg1;

    if (!context->isMapped(userQuery, userQuery + sizeof(OsNodeQueryInterfaceInfo), km::PageFlags::eUser | km::PageFlags::eRead)) {
        return km::CallError(OsStatusInvalidInput);
    }

    OsDeviceHandle device = OS_HANDLE_INVALID;
    sys2::InvokeContext invoke { system->sys, sys2::GetCurrentProcess() };
    if (OsStatus status = sys2::SysNodeQuery(&invoke, userHandle, *(OsNodeQueryInterfaceInfo*)userQuery, &device)) {
        return km::CallError(status);
    }

    return km::CallOk(device);
}

static OsCallResult NewNodeStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userHandle = regs->arg0;
    uint64_t userStat = regs->arg1;

    if (!context->isMapped(userStat, userStat + sizeof(OsNodeInfo), km::PageFlags::eUser | km::PageFlags::eWrite)) {
        return km::CallError(OsStatusInvalidInput);
    }

    sys2::InvokeContext invoke { system->sys, sys2::GetCurrentProcess() };
    if (OsStatus status = sys2::SysNodeStat(&invoke, userHandle, (OsNodeInfo*)userStat)) {
        return km::CallError(status);
    }

    return km::CallOk(0zu);
}

OsCallResult um::NodeOpen(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    if constexpr (kUseNewSystem) {
        return NewNodeOpen(system, context, regs);
    } else {
        return UserNodeOpen(system, context, regs);
    }
}

OsCallResult um::NodeClose(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    if constexpr (kUseNewSystem) {
        return NewNodeClose(system, context, regs);
    } else {
        return UserNodeClose(system, context, regs);
    }
}

OsCallResult um::NodeQuery(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    if constexpr (kUseNewSystem) {
        return NewNodeQuery(system, context, regs);
    } else {
        return UserNodeQuery(system, context, regs);
    }
}

OsCallResult um::NodeStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    if constexpr (kUseNewSystem) {
        return NewNodeStat(system, context, regs);
    } else {
        return UserNodeStat(system, context, regs);
    }
}
