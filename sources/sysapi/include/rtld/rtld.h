#pragma once

#include <bezos/facility/device.h>

#ifdef __cplusplus
extern "C" {
#endif

struct RtldStartInfo {
    /// @brief The file descriptor for the program.
    OsDeviceHandle Program;

    /// @brief The process to setup.
    OsProcessHandle Process;

    /// @brief The name of the program.
    OsUtf8Char ProgramName[OS_DEVICE_NAME_MAX];
};

struct RtldTlsInitInfo {
    void *InitAddress;
    OsSize TlsDataSize;
    OsSize TlsBssSize;
    OsSize TlsAlign;
};

struct RtldTls {
    void *BaseAddress;
    void *TlsAddress;
    OsSize Size;
    OsSize Align;
};

struct RtldSoLoadInfo {
    OsDeviceHandle Object;
};

struct RtldSo {
    OsDeviceHandle Object;
    RtldTlsInitInfo TlsInfo;
};

struct RtldSoName {
    const char *NameFront;
    const char *NameBack;
};

OsStatus RtldStartProgram(const struct RtldStartInfo *StartInfo, OsThreadHandle *OutThread);

OsStatus RtldCreateTls(OsProcessHandle Process, struct RtldTlsInitInfo *TlsInfo, RtldTls *OutTlsInfo);

OsStatus RtldSoOpen(const struct RtldSoLoadInfo *LoadInfo, struct RtldSo *OutObject);

OsStatus RtldSoClose(struct RtldSo *Object);

OsStatus RtldSoSymbol(struct RtldSo *Object, struct RtldSoName Name, void **OutAddress);

#ifdef __cplusplus
}
#endif
