#pragma once

#include <bezos/handle.h>

OS_BEGIN_API

OS_DEFINE_GUID(kOsFileGuid, 0x538202b8, 0xf97a, 0x11ef, 0x9446, 0x43376bcec51c);
OS_DEFINE_GUID(kOsFolderGuid, 0x5382039e, 0xf97a, 0x11ef, 0x9447, 0x73b093d39f67);

struct OsFileCreateInfo;

struct OsDeviceReadRequest;
struct OsDeviceWriteRequest;

struct OsUserContext;
struct OsUserHandle;

typedef OsStatus(*OsFileOpenEvent)(struct OsUserContext *Context, struct OsFileCreateInfo CreateInfo, struct OsUserHandle **OutHandle);
typedef OsStatus(*OsFileCloseEvent)(struct OsUserHandle *Handle);
typedef OsStatus(*OsFileReadEvent)(struct OsUserHandle *Handle, struct OsDeviceReadRequest ReadRequest, OsSize *OutRead);
typedef OsStatus(*OsFileWriteEvent)(struct OsUserHandle *Handle, struct OsDeviceWriteRequest WriteRequest, OsSize *OutWritten);

struct OsFileEvents {
    struct OsUserContext *Context;
    OsFileOpenEvent Open;
    OsFileCloseEvent Close;
    OsFileReadEvent Read;
    OsFileWriteEvent Write;
};

OS_END_API
