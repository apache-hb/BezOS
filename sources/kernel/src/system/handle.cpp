#include "system/system.hpp"
#include "system/process.hpp"

OsStatus sys2::SysHandleClose(InvokeContext *, HandleCloseInfo info) {
    ProcessHandle *target = info.process;
    if (!target->hasAccess(ProcessAccess::eIoControl)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Process> process = target->getProcess();
    if (OsStatus status = process->removeHandle(info.handle)) {
        return status;
    }

    delete info.handle;

    return OsStatusSuccess;
}

OsStatus sys2::SysHandleClone(InvokeContext *, HandleCloneInfo info, IHandle **handle) {
    ProcessHandle *target = info.process;
    IHandle *source = info.handle;
    IHandle *result = nullptr;

    if (!target->hasAccess(ProcessAccess::eIoControl)) {
        return OsStatusAccessDenied;
    }

    if (OsStatus status = source->clone(info.access, &result)) {
        return status;
    }

    sm::RcuSharedPtr<Process> process = target->getProcess();
    *handle = result;
    process->addHandle(result);

    return OsStatusSuccess;
}

OsStatus sys2::SysHandleStat(InvokeContext *, IHandle *handle, HandleStat *result) {
    *result = HandleStat {
        .access = handle->getAccess(),
    };

    return OsStatusSuccess;
}
