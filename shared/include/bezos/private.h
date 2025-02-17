#pragma once

#include <bezos/status.h>

struct OsCallResult {
    OsStatus Status;
    uint64_t Value;
};

enum {
    eOsCallFileOpen = 0x10,
    eOsCallFileClose = 0x11,
    eOsCallFileRead = 0x12,
    eOsCallFileWrite = 0x13,
    eOsCallFileSeek = 0x14,
    eOsCallFileStat = 0x15,
    eOsCallFileControl = 0x16,

    eOsCallDirIter = 0x20,
    eOsCallDirNext = 0x21,

    eOsCallProcessGetCurrent = 0x30,
    eOsCallProcessCreate = 0x31,
    eOsCallProcessExit = 0x32,

    eOsCallThreadGetCurrent = 0x40,
    eOsCallThreadCreate = 0x41,
    eOsCallThreadControl = 0x42,
    eOsCallThreadDestroy = 0x43,

    eOsCallTransactionBegin = 0x50,
    eOsCallTransactionCommit = 0x51,
    eOsCallTransactionRollback = 0x52,

    eOsCallQueryParam = 0x60,
    eOsCallUpdateParam = 0x61,

    eOsCallMutexCreate = 0x70,
    eOsCallMutexDestroy = 0x71,
    eOsCallMutexLock = 0x72,
    eOsCallMutexUnlock = 0x73,

    eOsCallDebugLog = 0x90,

    eOsCallCount = 0x100,
};

extern struct OsCallResult OsSystemCall(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
