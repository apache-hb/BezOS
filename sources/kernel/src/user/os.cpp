#include <bezos/facility/process.h>

#include "kernel.hpp"

#include "system/invoke.hpp"
#include "system/schedule.hpp"
#include "system/system.hpp"

static sys2::InvokeContext GetInvokeContext() {
    return sys2::InvokeContext { km::GetSysSystem(), sys2::GetCurrentProcess(), sys2::GetCurrentThread() };
}

extern "C" OsStatus OsProcessCreate(struct OsProcessCreateInfo CreateInfo, OsProcessHandle *OutHandle) {
    auto invoke = GetInvokeContext();
    return sys2::SysProcessCreate(&invoke, CreateInfo, OutHandle);
}

extern "C" OsStatus OsVmemCreate(struct OsVmemCreateInfo CreateInfo, OsAnyPointer *OutBase) {
    auto invoke = GetInvokeContext();
    return sys2::SysVmemCreate(&invoke, CreateInfo, OutBase);
}

extern "C" OsStatus OsVmemMap(struct OsVmemMapInfo MapInfo, OsAnyPointer *OutBase) {
    auto invoke = GetInvokeContext();
    return sys2::SysVmemMap(&invoke, MapInfo, OutBase);
}

extern "C" OsStatus OsVmemRelease(OsAnyPointer BaseAddress, OsSize Size) {
    auto invoke = GetInvokeContext();
    return sys2::SysVmemRelease(&invoke, BaseAddress, Size);
}

extern "C" OsStatus OsThreadCreate(struct OsThreadCreateInfo CreateInfo, OsThreadHandle *OutHandle) {
    auto invoke = GetInvokeContext();
    return sys2::SysThreadCreate(&invoke, CreateInfo, OutHandle);
}

extern "C" OsStatus OsThreadDestroy(OsThreadHandle Handle) {
    auto invoke = GetInvokeContext();
    return sys2::SysThreadDestroy(&invoke, eOsThreadFinished, Handle);
}

extern "C" OsStatus OsThreadSuspend(OsThreadHandle Handle, bool Suspend) {
    auto invoke = GetInvokeContext();
    return sys2::SysThreadSuspend(&invoke, Handle, Suspend);
}
