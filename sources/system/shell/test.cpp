#include <bezos/facility/process.h>
#include <bezos/facility/debug.h>
#include <bezos/facility/device.h>

#include <bezos/ext/args.h>
#include <posix/ext/args.h>

#include "common/bezos.hpp"

#include <iterator>

[[noreturn]]
void ProcessExit(int64_t err) {
    OsProcessTerminate(OS_HANDLE_INVALID, err);
    __builtin_unreachable();
}

OS_EXTERN OS_NORETURN
[[gnu::force_align_arg_pointer]]
void ClientStart(const struct OsClientStartInfo *) {
    OsProcessInfo info{};
    const OsProcessParam *param = nullptr;
    OsStatus status = OsStatusSuccess;

    status = OsProcessStat(OS_HANDLE_INVALID, &info);
    if (status != OsStatusSuccess) {
        OsDebugMessage(eOsLogError, "Failed to stat process\n");
        ProcessExit(1);
    }

    status = OsProcessFindArg(&info, &kPosixInitGuid, &param);
    if (status != OsStatusSuccess) {
        OsDebugMessage(eOsLogError, "Failed to find args\n");
        ProcessExit(1);
    }

    const OsPosixInitArgs *args = reinterpret_cast<const OsPosixInitArgs *>(param->Data);
    OsDeviceHandle standardOut = args->StandardOut;
    char message[] = "Hello, world!\n";
    OsDeviceWriteRequest request {
        .BufferFront = std::begin(message),
        .BufferBack = std::end(message),
        .Timeout = OS_TIMEOUT_INFINITE,
    };

    OsSize written = 0;
    status = OsDeviceWrite(standardOut, request, &written);
    if (status != OsStatusSuccess) {
        os::debug_message(eOsLogError, "Failed to write message: {:#x}", status);
        ProcessExit(1);
    }

    ProcessExit(0);
}
