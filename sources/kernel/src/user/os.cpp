#include <bezos/facility/process.h>

#include "kernel.hpp"

#include "system/invoke.hpp"
#include "system/schedule.hpp"
#include "system/system.hpp"

static sys::InvokeContext GetInvokeContext() {
    return sys::InvokeContext { km::GetSysSystem(), sys::GetCurrentProcess(), sys::GetCurrentThread() };
}

extern "C" OsStatus OsProcessCreate(struct OsProcessCreateInfo CreateInfo, OsProcessHandle *OutHandle) {
    auto invoke = GetInvokeContext();
    return sys::SysProcessCreate(&invoke, CreateInfo, OutHandle);
}

extern "C" OsStatus OsVmemCreate(struct OsVmemCreateInfo CreateInfo, OsAnyPointer *OutBase) {
    auto invoke = GetInvokeContext();
    return sys::SysVmemCreate(&invoke, CreateInfo, OutBase);
}

extern "C" OsStatus OsVmemMap(struct OsVmemMapInfo MapInfo, OsAnyPointer *OutBase) {
    auto invoke = GetInvokeContext();
    return sys::SysVmemMap(&invoke, MapInfo, OutBase);
}

extern "C" OsStatus OsVmemRelease(OsAnyPointer BaseAddress, OsSize Size) {
    auto invoke = GetInvokeContext();
    return sys::SysVmemRelease(&invoke, BaseAddress, Size);
}

extern "C" OsStatus OsThreadCreate(struct OsThreadCreateInfo CreateInfo, OsThreadHandle *OutHandle) {
    auto invoke = GetInvokeContext();
    return sys::SysThreadCreate(&invoke, CreateInfo, OutHandle);
}

extern "C" OsStatus OsThreadDestroy(OsThreadHandle Handle) {
    auto invoke = GetInvokeContext();
    return sys::SysThreadDestroy(&invoke, eOsThreadFinished, Handle);
}

extern "C" OsStatus OsThreadSuspend(OsThreadHandle Handle, bool Suspend) {
    auto invoke = GetInvokeContext();
    return sys::SysThreadSuspend(&invoke, Handle, Suspend);
}
