#include "system/create.hpp"
#include "system/schedule.hpp"
#include "system/system.hpp"
#include "user/sysapi.hpp"

#include "syscall.hpp"

static OsCallResult NewDeviceOpen(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userCreateInfo = regs->arg0;

    OsDeviceCreateInfo createInfo{};
    if (OsStatus status = context->readObject(userCreateInfo, &createInfo)) {
        return km::CallError(status);
    }

    sm::uuid uuid = createInfo.InterfaceGuid;

    vfs2::VfsPath path;
    if (OsStatus status = um::ReadPath(context, createInfo.Path, &path)) {
        return km::CallError(status);
    }

    sys::InvokeContext invoke { system->sys, sys::GetCurrentProcess() };
    sys::DeviceOpenInfo openInfo {
        .path = path,
        .flags = createInfo.Flags,
        .interface = uuid,
    };

    OsDeviceHandle handle = OS_HANDLE_INVALID;
    if (OsStatus status = sys::SysDeviceOpen(&invoke, openInfo, &handle)) {
        return km::CallError(status);
    }

    return km::CallOk(handle);
}

static OsCallResult NewDeviceClose(km::System *system, km::CallContext *, km::SystemCallRegisterSet *regs) {
    uint64_t userDevice = regs->arg0;

    sys::InvokeContext invoke { system->sys, sys::GetCurrentProcess() };
    if (OsStatus status = sys::SysDeviceClose(&invoke, userDevice)) {
        return km::CallError(status);
    }

    return km::CallOk(0zu);
}

static OsCallResult NewDeviceRead(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userHandle = regs->arg0;
    uint64_t userRequest = regs->arg1;

    OsDeviceReadRequest request{};
    if (OsStatus status = context->readObject(userRequest, &request)) {
        return km::CallError(status);
    }

    if (!context->isMapped((uint64_t)request.BufferFront, (uint64_t)request.BufferBack, km::PageFlags::eUser | km::PageFlags::eWrite)) {
        return km::CallError(OsStatusInvalidInput);
    }

    sys::InvokeContext invoke { system->sys, sys::GetCurrentProcess() };
    OsSize read = 0;

    if (OsStatus status = sys::SysDeviceRead(&invoke, userHandle, request, &read)) {
        return km::CallError(status);
    }

    return km::CallOk(read);
}

static OsCallResult NewDeviceWrite(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userHandle = regs->arg0;
    uint64_t userRequest = regs->arg1;

    OsDeviceWriteRequest request{};
    if (OsStatus status = context->readObject(userRequest, &request)) {
        return km::CallError(status);
    }

    if (!context->isMapped((uint64_t)request.BufferFront, (uint64_t)request.BufferBack, km::PageFlags::eUser | km::PageFlags::eRead)) {
        return km::CallError(OsStatusInvalidInput);
    }

    sys::InvokeContext invoke { system->sys, sys::GetCurrentProcess() };
    OsSize write = 0;

    if (OsStatus status = sys::SysDeviceWrite(&invoke, userHandle, request, &write)) {
        return km::CallError(status);
    }

    return km::CallOk(write);
}

static OsCallResult NewDeviceInvoke(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userHandle = regs->arg0;
    uint64_t userFunction = regs->arg1;
    uint64_t userData = regs->arg2;
    uint64_t userSize = regs->arg3;

    if (!context->isMapped((uint64_t)userData, (uint64_t)userData + userSize, km::PageFlags::eUserData)) {
        return km::CallError(OsStatusInvalidInput);
    }

    sys::InvokeContext invoke { system->sys, sys::GetCurrentProcess() };
    if (OsStatus status = sys::SysDeviceInvoke(&invoke, userHandle, userFunction, (void*)userData, userSize)) {
        return km::CallError(status);
    }

    return km::CallOk(0zu);
}

static OsCallResult NewDeviceStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userHandle = regs->arg0;
    uint64_t userStat = regs->arg1;

    if (!context->isMapped(userStat, userStat + sizeof(OsDeviceInfo), km::PageFlags::eUserData)) {
        return km::CallError(OsStatusInvalidInput);
    }

    sys::InvokeContext invoke { system->sys, sys::GetCurrentProcess() };
    if (OsStatus status = sys::SysDeviceStat(&invoke, userHandle, (OsDeviceInfo*)userStat)) {
        return km::CallError(status);
    }

    return km::CallOk(0zu);
}

OsCallResult um::DeviceOpen(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    return NewDeviceOpen(system, context, regs);
}

OsCallResult um::DeviceClose(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    return NewDeviceClose(system, context, regs);
}

OsCallResult um::DeviceRead(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    return NewDeviceRead(system, context, regs);
}

OsCallResult um::DeviceWrite(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    return NewDeviceWrite(system, context, regs);
}

OsCallResult um::DeviceInvoke(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    return NewDeviceInvoke(system, context, regs);
}

OsCallResult um::DeviceStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    return NewDeviceStat(system, context, regs);
}
