#pragma once

#include <bezos/status.h>

struct OsCallResult {
    OsStatus Status;
    uint64_t Value;
};

enum {
    eOsCallHandleWait = 0x1,
    eOsCallDebugMessage = 0x2,

    eOsCallFileOpen = 0x10,
    eOsCallFileClose = 0x11,
    eOsCallFileRead = 0x12,
    eOsCallFileWrite = 0x13,
    eOsCallFileSeek = 0x14,
    eOsCallFileStat = 0x15,

    eOsCallFolderIterateCreate = 0x20,
    eOsCallFolderIterateDestroy = 0x21,
    eOsCallFolderIterateNext = 0x22,

    eOsCallProcessCreate = 0x30,
    eOsCallProcessDestroy = 0x31,
    eOsCallProcessCurrent = 0x32,
    eOsCallProcessSuspend = 0x33,

    eOsCallThreadCreate = 0x40,
    eOsCallThreadDestroy = 0x41,
    eOsCallThreadCurrent = 0x42,
    eOsCallThreadSleep = 0x43,
    eOsCallThreadStat = 0x44,

    eOsCallVmemCreate = 0x53,

    eOsCallTransactionBegin = 0x60,
    eOsCallTransactionCommit = 0x61,
    eOsCallTransactionRollback = 0x62,

    eOsCallMutexCreate = 0x70,
    eOsCallMutexDestroy = 0x71,
    eOsCallMutexLock = 0x72,
    eOsCallMutexUnlock = 0x73,

    eOsCallDeviceOpen = 0x80,
    eOsCallDeviceClose = 0x81,
    eOsCallDeviceRead = 0x82,
    eOsCallDeviceWrite = 0x83,
    eOsCallDeviceInvoke = 0x84,
    eOsCallDeviceQuery = 0x85,

    eOsCallClockGetTime = 0x90,
    eOsCallClockStat = 0x91,

    eOsCallCount = 0xFF,
};

extern struct OsCallResult OsSystemCall(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
