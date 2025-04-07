#pragma once

#include <bezos/facility/process.h>
#include <bezos/facility/vmem.h>
#include <bezos/facility/tx.h>
#include <bezos/facility/mutex.h>
#include <bezos/facility/threads.h>
#include <bezos/facility/device.h>
#include <bezos/facility/fs.h>

#include "util/util.hpp"

namespace sys2 {
    class System;
    class IObject;
    class IHandle;
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
        eWait = eOsProcessAccessWait,
        eTerminate = eOsProcessAccessTerminate,
        eStat = eOsProcessAccessStat,
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
        eWait = eOsTxAccessWait,
        eCommit = eOsTxAccessCommit,
        eRollback = eOsTxAccessRollback,
        eStat = eOsTxAccessStat,
        eWrite = eOsTxAccessWrite,
        eAll = eOsTxAccessAll,
    };

    UTIL_BITFLAGS(TxAccess);

    enum class MutexAccess : OsHandleAccess {
        eNone = eOsMutexAccessNone,
        eWait = eOsMutexAccessWait,
        eDestroy = eOsMutexAccessDestroy,
        eUpdate = eOsMutexAccessUpdate,
        eStat = eOsMutexAccessStat,
        eAll = eOsMutexAccessAll,
    };

    UTIL_BITFLAGS(MutexAccess);

    enum class ThreadAccess : OsHandleAccess {
        eNone = eOsThreadAccessNone,
        eWait = eOsThreadAccessWait,
        eTerminate = eOsThreadAccessTerminate,
        eStat = eOsThreadAccessStat,
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
        eInvoke = eOsDeviceAccessInvoke,
        eAll = eOsDeviceAccessAll,
    };

    UTIL_BITFLAGS(DeviceAccess);

    enum class NodeAccess : OsHandleAccess {
        eNone = eOsNodeAccessNone,
        eRead = eOsNodeAccessRead,
        eWrite = eOsNodeAccessWrite,
        eStat = eOsNodeAccessStat,
        eDestroy = eOsNodeAccessDestroy,
        eQueryInterface = eOsNodeAccessQueryInterface,
        eAll = eOsNodeAccessAll,
    };

    UTIL_BITFLAGS(NodeAccess);
}
