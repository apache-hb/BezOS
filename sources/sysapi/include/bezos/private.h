#pragma once

#include <bezos/status.h>

struct OsCallResult {
    OsStatus Status;
    uint64_t Value;
};

enum {
    eOsCallHandleWait = 0x1,
    eOsCallDebugMessage = 0x2,

    eOsCallNodeOpen = 0x16,
    eOsCallNodeClose = 0x17,
    eOsCallNodeQuery = 0x18,
    eOsCallNodeStat = 0x19,

    eOsCallProcessCreate = 0x30,
    eOsCallProcessDestroy = 0x31,
    eOsCallProcessCurrent = 0x32,
    eOsCallProcessSuspend = 0x33,
    eOsCallProcessStat = 0x34,

    eOsCallThreadCreate = 0x40,
    eOsCallThreadDestroy = 0x41,
    eOsCallThreadCurrent = 0x42,
    eOsCallThreadSleep = 0x43,
    eOsCallThreadSuspend = 0x44,
    eOsCallThreadStat = 0x45,

    eOsCallVmemCreate = 0x50,
    eOsCallVmemRelease = 0x51,

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
    eOsCallDeviceStat = 0x86,

    eOsCallClockGetTime = 0x90,
    eOsCallClockStat = 0x91,
    eOsCallClockTicks = 0x92,

    eOsCallCount = 0xFF,
};

extern struct OsCallResult OsSystemCall(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
