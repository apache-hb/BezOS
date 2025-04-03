#pragma once

#include <bezos/facility/device.h>

#ifdef __cplusplus
extern "C" {
#endif

struct RtldStartInfo {
    /// @brief The file descriptor for the program.
    OsDeviceHandle Program;

    /// @brief The name of the program.
    OsUtf8Char ProgramName[OS_DEVICE_NAME_MAX];
};

struct RtldSoLoadInfo {
    OsDeviceHandle Object;
};

struct RtldSo {
    OsDeviceHandle Object;
};

struct RtldSoName {
    const char *NameFront;
    const char *NameBack;
};

OsStatus RtldStartProgram(const struct RtldStartInfo *StartInfo);

OsStatus RtldSoOpen(const struct RtldSoLoadInfo *LoadInfo, struct RtldSo *OutObject);

OsStatus RtldSoClose(struct RtldSo *Object);
OsStatus RtldSoSymbol(struct RtldSharedObject *Object, RtldSoName Name, void **OutAddress);

#ifdef __cplusplus
}
#endif
