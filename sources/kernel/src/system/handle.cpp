#include "system/system.hpp"
#include "system/process.hpp"

OsStatus sys2::SysHandleClose(InvokeContext *context, OsHandle handle) {
    if (OsStatus status = context->process->removeHandle(handle)) {
        return status;
    }

    return OsStatusSuccess;
}

OsStatus sys2::SysHandleClone(InvokeContext *context, OsHandle handle, OsHandleCloneInfo info, OsHandle *outHandle) {
    IHandle *source = context->process->getHandle(handle);
    if (!source) {
        return OsStatusInvalidHandle;
    }

    OsHandle id = context->process->newHandleId(source->getHandleType());
    IHandle *result = nullptr;
    if (OsStatus status = source->clone(info.Access, id, &result)) {
        return status;
    }

    context->process->addHandle(result);
    *outHandle = result->getHandle();

    return OsStatusSuccess;
}

OsStatus sys2::SysHandleStat(InvokeContext *context, OsHandle handle, HandleStat *result) {
    IHandle *source = context->process->getHandle(handle);
    if (!source) {
        return OsStatusInvalidHandle;
    }

    *result = HandleStat {
        .access = source->getAccess(),
    };

    return OsStatusSuccess;
}
