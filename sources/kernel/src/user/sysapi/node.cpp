#include "log.hpp"
#include "user/sysapi.hpp"

#include "process/device.hpp"

#include "process/system.hpp"
#include "fs2/vfs.hpp"
#include "syscall.hpp"

// TODO: make this runtime configurable
static constexpr size_t kMaxPathSize = 0x1000;

OsStatus um::UserReadPath(km::CallContext *context, OsPath user, vfs2::VfsPath *path) {
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

OsCallResult um::NodeOpen(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userCreateInfo = regs->arg0;

    km::Process *process = nullptr;
    vfs2::INode *node = nullptr;
    km::Node *result = nullptr;
    vfs2::VfsPath path;
    OsNodeCreateInfo createInfo{};

    if (OsStatus status = context->readObject(userCreateInfo, &createInfo)) {
        return km::CallError(status);
    }

    if (OsStatus status = SelectOwningProcess(system, context, createInfo.Process, &process)) {
        return km::CallError(status);
    }

    if (OsStatus status = UserReadPath(context, createInfo.Path, &path)) {
        return km::CallError(status);
    }

    if (OsStatus status = system->vfs->lookup(path, &node)) {
        return km::CallError(status);
    }

    if (OsNodeHandle existing = system->objects->getNodeId(node)) {
        return km::CallOk(existing);
    }

    if (OsStatus status = system->objects->createNode(process, std::move(node), &result)) {
        return km::CallError(status);
    }

    return km::CallOk(result->publicId());
}

OsCallResult um::NodeClose(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
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

OsCallResult um::NodeQuery(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
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

OsCallResult um::NodeStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
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
