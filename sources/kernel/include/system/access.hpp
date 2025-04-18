#pragma once

#include <bezos/facility/process.h>
#include <bezos/facility/vmem.h>
#include <bezos/facility/tx.h>
#include <bezos/facility/mutex.h>
#include <bezos/facility/threads.h>
#include <bezos/facility/device.h>
#include <bezos/facility/fs.h>
#include <bezos/facility/event.h>

#include "common/util/util.hpp"

namespace sys2 {
    class System;
    class IObject;
    class IHandle;
    class IIsolate;
    class InvokeContext;
    class Tx;
    class TxHandle;
    class Process;
    class ProcessHandle;
    class Mutex;
    class MutexHandle;
    class Thread;
    class ThreadHandle;
    class Device;
    class DeviceHandle;
    class Node;
    class NodeHandle;

    enum class ProcessAccess : OsHandleAccess {
        eNone = eOsProcessAccessNone,

        eRead = eOsProcessAccessRead,
        eWrite = eOsProcessAccessWrite,
        eStat = eOsProcessAccessStat,
        eDestroy = eOsProcessAccessDestroy,
        eWait = eOsProcessAccessWait,

        eSuspend = eOsProcessAccessSuspend,
        eVmControl = eOsProcessAccessVmControl,
        eThreadControl = eOsProcessAccessThreadControl,
        eIoControl = eOsProcessAccessIoControl,
        eProcessControl = eOsProcessAccessProcessControl,
        eQuota = eOsProcessAccessQuota,
        eTxControl = eOsProcessTxControl,

        eAll = eOsProcessAccessAll,
    };

    UTIL_BITFLAGS(ProcessAccess);

    enum class TxAccess : OsHandleAccess {
        eNone = eOsTxAccessNone,

        eRead = eOsTxAccessRead,
        eWrite = eOsTxAccessWrite,
        eStat = eOsTxAccessStat,
        eDestroy = eOsTxAccessDestroy,
        eWait = eOsTxAccessWait,

        eCommit = eOsTxAccessCommit,
        eRollback = eOsTxAccessRollback,

        eAll = eOsTxAccessAll,
    };

    UTIL_BITFLAGS(TxAccess);

    enum class MutexAccess : OsHandleAccess {
        eNone = eOsMutexAccessNone,

        eRead = eOsMutexAccessRead,
        eWrite = eOsMutexAccessWrite,
        eStat = eOsMutexAccessStat,
        eDestroy = eOsMutexAccessDestroy,
        eWait = eOsMutexAccessWait,

        eLock = eOsMutexAccessLock,
        eLockShared = eOsMutexAccessLockShared,

        eAll = eOsMutexAccessAll,
    };

    UTIL_BITFLAGS(MutexAccess);

    enum class ThreadAccess : OsHandleAccess {
        eNone = eOsThreadAccessNone,

        eRead = eOsThreadAccessRead,
        eWrite = eOsThreadAccessWrite,
        eStat = eOsThreadAccessStat,
        eDestroy = eOsThreadAccessDestroy,
        eWait = eOsThreadAccessWait,

        eSuspend = eOsThreadAccessSuspend,
        eQuota = eOsThreadAccessQuota,

        eAll = eOsThreadAccessAll,
    };

    UTIL_BITFLAGS(ThreadAccess);

    enum class DeviceAccess : OsHandleAccess {
        eNone = eOsDeviceAccessNone,

        eRead = eOsDeviceAccessRead,
        eWrite = eOsDeviceAccessWrite,
        eStat = eOsDeviceAccessStat,
        eDestroy = eOsDeviceAccessDestroy,
        eWait = eOsDeviceAccessWait,

        eInvoke = eOsDeviceAccessInvoke,

        eAll = eOsDeviceAccessAll,

        R = eRead,
        W = eWrite,
        X = eInvoke,
        RW = R | W,
        RX = R | X,
        RWX = R | W | X,
    };

    UTIL_BITFLAGS(DeviceAccess);

    enum class NodeAccess : OsHandleAccess {
        eNone = eOsNodeAccessNone,

        eRead = eOsNodeAccessRead,
        eWrite = eOsNodeAccessWrite,
        eStat = eOsNodeAccessStat,
        eDestroy = eOsNodeAccessDestroy,
        eWait = eOsNodeAccessWait,

        eExecute = eOsNodeAccessExecute,
        eQueryInterface = eOsNodeAccessQueryInterface,

        eAll = eOsNodeAccessAll,

        R = eRead,
        W = eWrite,
        X = eExecute,
        RW = R | W,
        RX = R | X,
        RWX = R | W | X,
    };

    UTIL_BITFLAGS(NodeAccess);

    enum class EventAccess : OsHandleAccess {
        eNone = eOsEventAccessNone,

        eRead = eOsEventAccessRead,
        eWrite = eOsEventAccessWrite,
        eStat = eOsEventAccessStat,
        eDestroy = eOsEventAccessDestroy,
        eWait = eOsEventAccessWait,

        eSignal = eOsEventAccessSignal,

        eAll = eOsEventAccessAll,
    };

    UTIL_BITFLAGS(EventAccess);
}
