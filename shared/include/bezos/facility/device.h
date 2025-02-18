#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

struct OsDeviceCreateInfo {
    const char *NameFront;
    const char *NameBack;

    struct OsGuid InterfaceGuid;
};

struct OsDeviceReadRequest {
    void *BufferFront;
    void *BufferBack;
    OsInstant Timeout;
};

struct OsDeviceWriteRequest {
    const void *BufferFront;
    const void *BufferBack;
    OsInstant Timeout;
};

struct OsDeviceCallRequest {
    uint64_t Function;
    const void *InputFront;
    const void *InputBack;

    void *OutputFront;
    void *OutputBack;
};

extern OsStatus OsDeviceOpen(struct OsDeviceCreateInfo CreateInfo, OsDeviceHandle *OutHandle);

extern OsStatus OsDeviceClose(OsDeviceHandle Handle);

extern OsStatus OsDeviceRead(OsDeviceHandle Handle, struct OsDeviceReadRequest Request, OsSize *OutRead);

extern OsStatus OsDeviceWrite(OsDeviceHandle Handle, struct OsDeviceWriteRequest Request, OsSize *OutWritten);

extern OsStatus OsDeviceCall(OsDeviceHandle Handle, uint64_t Function, void *Request);

#ifdef __cplusplus
}
#endif
