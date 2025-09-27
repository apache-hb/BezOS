#include "logger/categories.hpp"
#include "system/create.hpp"
#include "system/schedule.hpp"
#include "system/system.hpp"
#include "user/sysapi.hpp"

#include "syscall.hpp"

// TODO: make this runtime configurable
static constexpr size_t kMaxPathSize = 0x1000;

OsStatus um::ReadPath(km::CallContext *context, OsPath user, vfs::VfsPath *path) {
    vfs::VfsString text;
    if (OsStatus status = context->readString((uint64_t)user.Front, (uint64_t)user.Back, kMaxPathSize, &text)) {
        return status;
    }

    if (!vfs::VerifyPathText(text)) {
        UserLog.dbgf("[PROC] Invalid path: '", text, "'");
        return OsStatusInvalidPath;
    }

    *path = vfs::VfsPath{std::move(text)};
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

static OsCallResult NewNodeOpen(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userCreateInfo = regs->arg0;

    vfs::VfsPath path;
    OsNodeCreateInfo createInfo{};

    if (OsStatus status = context->readObject(userCreateInfo, &createInfo)) {
        return km::CallError(status);
    }

    if (OsStatus status = um::ReadPath(context, createInfo.Path, &path)) {
        return km::CallError(status);
    }

    sys::InvokeContext invoke { system->sys, sys::GetCurrentProcess() };
    sys::NodeOpenInfo openInfo {
        .path = path,
    };

    OsNodeHandle node = OS_HANDLE_INVALID;

    if (OsStatus status = sys::SysNodeOpen(&invoke, openInfo, &node)) {
        return km::CallError(status);
    }

    return km::CallOk(node);
}

static OsCallResult NewNodeClose(km::System *system, km::CallContext *, km::SystemCallRegisterSet *regs) {
    uint64_t userHandle = regs->arg0;

    sys::InvokeContext invoke { system->sys, sys::GetCurrentProcess() };
    if (OsStatus status = sys::SysNodeClose(&invoke, userHandle)) {
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
    sys::InvokeContext invoke { system->sys, sys::GetCurrentProcess() };
    if (OsStatus status = sys::SysNodeQuery(&invoke, userHandle, *(OsNodeQueryInterfaceInfo*)userQuery, &device)) {
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

    sys::InvokeContext invoke { system->sys, sys::GetCurrentProcess() };
    if (OsStatus status = sys::SysNodeStat(&invoke, userHandle, (OsNodeInfo*)userStat)) {
        return km::CallError(status);
    }

    return km::CallOk(0zu);
}

OsCallResult um::NodeOpen(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    return NewNodeOpen(system, context, regs);
}

OsCallResult um::NodeClose(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    return NewNodeClose(system, context, regs);
}

OsCallResult um::NodeQuery(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    return NewNodeQuery(system, context, regs);
}

OsCallResult um::NodeStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    return NewNodeStat(system, context, regs);
}
