#include "system/system.hpp"
#include "system/process.hpp"

OsStatus sys2::SysHandleClose(InvokeContext *context, OsHandle handle) {
    ProcessHandle *target = context->process;
    if (!target->hasAccess(ProcessAccess::eIoControl)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Process> process = target->getProcess();
    if (OsStatus status = process->removeHandle(handle)) {
        return status;
    }

    return OsStatusSuccess;
}

OsStatus sys2::SysHandleClone(InvokeContext *context, OsHandle handle, OsHandleCloneInfo info, OsHandle *outHandle) {
    ProcessHandle *target = context->process;
    if (!target->hasAccess(ProcessAccess::eIoControl)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Process> process = target->getProcess();
    IHandle *source = process->getHandle(handle);
    if (!source) {
        return OsStatusInvalidHandle;
    }

    OsHandle id = process->newHandleId(source->getHandleType());
    IHandle *result = nullptr;
    if (OsStatus status = source->clone(info.Access, id, &result)) {
        return status;
    }

    process->addHandle(result);
    *outHandle = result->getHandle();

    return OsStatusSuccess;
}

OsStatus sys2::SysHandleStat(InvokeContext *context, OsHandle handle, HandleStat *result) {
    ProcessHandle *target = context->process;
    if (!target->hasAccess(ProcessAccess::eIoControl)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Process> process = target->getProcess();
    IHandle *source = process->getHandle(handle);
    if (!source) {
        return OsStatusInvalidHandle;
    }

    *result = HandleStat {
        .access = source->getAccess(),
    };

    return OsStatusSuccess;
}
