#pragma once

#include <bezos/handle.h>

#include "std/vector.hpp"
#include "std/rcuptr.hpp"

#include "memory/layout.hpp"
#include "system/create.hpp"
#include "system/query.hpp"
#include "system/schedule.hpp"

#include <compare> // IWYU pragma: keep
#include <queue>

namespace km {
    class PageTables;
    class PageAllocator;
    class AddressSpace;
}

namespace sys2 {
    class IObject;
    class Process;
    class Thread;
    class GlobalSchedule;

    struct SleepEntry {
        OsInstant wake;
        sm::RcuWeakPtr<Thread> thread;

        constexpr auto operator<=>(const SleepEntry& other) const noexcept {
            return wake <=> other.wake;
        }
    };

    struct WaitEntry {
        /// @brief Timeout when this entry should be completed
        OsInstant timeout;

        /// @brief The thread that is waiting
        sm::RcuWeakPtr<Thread> thread;

        /// @brief The object that is being waited on
        sm::RcuWeakPtr<IObject> object;

        constexpr auto operator<=>(const WaitEntry& other) const noexcept {
            return timeout <=> other.timeout;
        }
    };

    using WaitQueue = std::priority_queue<WaitEntry>;

    class System {
    public:
        stdx::SharedSpinLock mLock;
        sm::RcuDomain mDomain;

        GlobalSchedule mSchedule;

        km::AddressSpace *mSystemTables;

        km::PageAllocator *mPageAllocator;

        sm::FlatHashMap<sm::RcuWeakPtr<IObject>, WaitQueue> mWaitQueue;
        std::priority_queue<WaitEntry> mTimeoutQueue;
        std::priority_queue<SleepEntry> mSleepQueue;

        sm::FlatHashSet<sm::RcuSharedPtr<IObject>, sm::RcuHash<IObject>, std::equal_to<>> mObjects GUARDED_BY(mLock);

        sm::FlatHashSet<sm::RcuSharedPtr<Process>, sm::RcuHash<Process>, std::equal_to<>> mProcessObjects GUARDED_BY(mLock);

    public:
        System(GlobalScheduleCreateInfo info, km::AddressSpace *systemTables [[gnu::nonnull]], km::PageAllocator *pmm [[gnu::nonnull]])
            : mSchedule(info)
            , mSystemTables(systemTables)
            , mPageAllocator(pmm)
        { }

        sm::RcuDomain &rcuDomain() { return mDomain; }

        void addObject(sm::RcuSharedPtr<IObject> object);
        void removeObject(sm::RcuWeakPtr<IObject> object);

        void addProcessObject(sm::RcuSharedPtr<Process> object);
        void removeProcessObject(sm::RcuWeakPtr<Process> object);

        void addThreadObject(sm::RcuSharedPtr<Thread> object);
        void removeThreadObject(sm::RcuWeakPtr<Thread> object);

        OsStatus mapProcessPageTables(km::AddressMapping *mapping);
        OsStatus mapSystemStack(km::StackMapping *mapping);

        void releaseMemory(km::MemoryRange range);
        OsStatus releaseMapping(km::AddressMapping mapping);
        OsStatus releaseStack(km::StackMapping mapping);

        km::AddressSpace *pageTables() { return mSystemTables; }

        OsStatus createProcess(ProcessCreateInfo info, ProcessHandle **handle);
        OsStatus createThread(ThreadCreateInfo info, ThreadHandle **handle);
        OsStatus createTx(TxCreateInfo info, TxHandle **handle);
        OsStatus createMutex(MutexCreateInfo info, MutexHandle **handle);

        OsStatus getProcessList(stdx::Vector2<sm::RcuSharedPtr<Process>>& list);
    };

    // internal

    OsStatus SysCreateRootProcess(System *system, ProcessCreateInfo info, ProcessHandle **handle);

    // handle

    OsStatus SysHandleClose(InvokeContext *context, IHandle *handle);
    OsStatus SysHandleClone(InvokeContext *context, HandleCloneInfo info, IHandle **handle);
    OsStatus SysHandleStat(InvokeContext *context, IHandle *handle, HandleStat *result);

    // node

    OsStatus SysNodeOpen(InvokeContext *context, NodeOpenInfo info, NodeHandle **handle);
    OsStatus SysNodeClose(InvokeContext *context, NodeCloseInfo info);
    OsStatus SysNodeQuery(InvokeContext *context, NodeQueryInfo info, NodeQueryResult *result);
    OsStatus SysNodeStat(InvokeContext *context, NodeStatInfo info, NodeStatResult *result);

    // device

    OsStatus SysDeviceOpen(InvokeContext *context, DeviceOpenInfo info, DeviceHandle **handle);
    OsStatus SysDeviceClose(InvokeContext *context, DeviceCloseInfo info);
    OsStatus SysDeviceRead(InvokeContext *context, DeviceReadInfo info, DeviceReadResult *result);
    OsStatus SysDeviceWrite(InvokeContext *context, DeviceWriteInfo info, DeviceWriteResult *result);
    OsStatus SysDeviceInvoke(InvokeContext *context, DeviceInvokeInfo info, DeviceInvokeResult *result);
    OsStatus SysDeviceStat(InvokeContext *context, DeviceStatInfo info, DeviceStatResult *result);

    // process

    OsStatus SysCreateProcess(InvokeContext *context, ProcessCreateInfo info, ProcessHandle **handle);
    OsStatus SysDestroyProcess(InvokeContext *context, ProcessDestroyInfo info);
    OsStatus SysProcessStat(InvokeContext *context, ProcessStatInfo info, ProcessStatResult *result);

    OsStatus SysQueryProcessList(InvokeContext *context, ProcessQueryInfo info, ProcessQueryResult *result);

    // vmem

    OsStatus SysCreateVmem(InvokeContext *context, VmemCreateInfo info);
    OsStatus SysReleaseVmem(InvokeContext *context, VmemReleaseInfo info);
    OsStatus SysMapVmem(InvokeContext *context, VmemMapInfo info);

    // thread

    OsStatus SysCreateThread(InvokeContext *context, ThreadCreateInfo info, ThreadHandle **handle);
    OsStatus SysDestroyThread(InvokeContext *context, ThreadDestroyInfo info);
    OsStatus SysThreadStat(InvokeContext *context, ThreadHandle *handle, ThreadStat *result);

    // tx

    OsStatus SysCreateTx(InvokeContext *context, TxCreateInfo info, TxHandle **handle);
    OsStatus SysCommitTx(InvokeContext *context, TxDestroyInfo info);
    OsStatus SysAbortTx(InvokeContext *context, TxDestroyInfo info);

    // mutex

    OsStatus SysCreateMutex(InvokeContext *context, MutexCreateInfo info, MutexHandle **handle);
    OsStatus SysDestroyMutex(InvokeContext *context, MutexDestroyInfo info);
}
