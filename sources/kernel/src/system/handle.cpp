#include "system/system.hpp"
#include "system/process.hpp"

OsStatus sys::SysHandleClose(InvokeContext *context, OsHandle handle) {
    if (OsStatus status = context->process->removeHandle(handle)) {
        return status;
    }

    return OsStatusSuccess;
}

OsStatus sys::SysHandleClone(InvokeContext *context, OsHandle handle, OsHandleCloneInfo info, OsHandle *outHandle) {
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

OsStatus sys::SysHandleStat(InvokeContext *context, OsHandle handle, OsHandleInfo *result) {
    IHandle *source = context->process->getHandle(handle);
    if (!source) {
        return OsStatusInvalidHandle;
    }

    if (!source->hasGenericAccess(eOsAccessStat)) {
        return OsStatusAccessDenied;
    }

    *result = OsHandleInfo {
        .Access = source->getAccess(),
    };

    return OsStatusSuccess;
}

OsStatus sys::SysHandleWait(InvokeContext *context, OsHandle handle, OsInstant timeout) {
    return OsStatusNotSupported; // TODO: Implement handle wait

    // IHandle *source = context->process->getHandle(handle);
    // if (!source) {
    //     return OsStatusInvalidHandle;
    // }

    // if (source->hasGenericAccess(eOsAccessWait)) {
    //     return OsStatusAccessDenied;
    // }

    // auto weak = source->getObject();
    // auto object = weak.lock();
    // if (!object) {
    //     return OsStatusInvalidHandle;
    // }

    // auto schedule = context->system->scheduler();
    // return schedule->wait(context->thread, object, timeout);
}
