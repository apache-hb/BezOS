#pragma once

#include <bezos/handle.h>
#include <bezos/facility/handle.h>

#include "std/vector.hpp"
#include "std/rcuptr.hpp"

#include "memory/layout.hpp"
#include "system/create.hpp"
#include "system/query.hpp"
#include "system/schedule.hpp"

#include <compare> // IWYU pragma: keep

namespace km {
    class PageTables;
    class PageAllocator;
    class AddressSpace;
}

namespace vfs2 {
    class VfsRoot;
    class INode;
}

namespace sys2 {
    class System {
        std::atomic<OsProcessId> mPidCounter{1};
    public:
        stdx::SharedSpinLock mLock;
        sm::RcuDomain mDomain;

        GlobalSchedule mSchedule;

        km::AddressSpace *mSystemTables;

        km::PageAllocator *mPageAllocator;

        vfs2::VfsRoot *mVfsRoot;

        sm::FlatHashSet<sm::RcuSharedPtr<IObject>, sm::RcuHash<IObject>, std::equal_to<>> mObjects GUARDED_BY(mLock);

        sm::FlatHashSet<sm::RcuSharedPtr<Process>, sm::RcuHash<Process>, std::equal_to<>> mProcessObjects GUARDED_BY(mLock);

    public:
        System( km::AddressSpace *systemTables [[gnu::nonnull]], km::PageAllocator *pmm [[gnu::nonnull]], vfs2::VfsRoot *vfsRoot)
            : mSystemTables(systemTables)
            , mPageAllocator(pmm)
            , mVfsRoot(vfsRoot)
        { }

        sm::RcuDomain &rcuDomain() { return mDomain; }

        void addThreadObject(sm::RcuSharedPtr<Thread> object);
        void removeThreadObject(sm::RcuWeakPtr<Thread> object);

        void addProcessObject(sm::RcuSharedPtr<Process> object);
        void removeProcessObject(sm::RcuWeakPtr<Process> object);

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

        SystemStats stats();

        OsProcessId nextProcessId() {
            return mPidCounter.fetch_add(1);
        }

        GlobalSchedule *scheduler() {
            return &mSchedule;
        }

        CpuLocalSchedule *getCpuSchedule(km::CpuCoreId cpu) {
            return mSchedule.getCpuSchedule(cpu);
        }
    };

    // internal

    OsStatus SysCreateRootProcess(System *system, ProcessCreateInfo info, ProcessHandle **handle);
    OsStatus SysDestroyRootProcess(System *system, ProcessHandle *handle);
    OsStatus SysResolveObject(InvokeContext *context, sm::RcuSharedPtr<IObject> object, OsHandleAccess access, OsHandle *handle);

    // handle

    OsStatus SysHandleClose(InvokeContext *context, OsHandle handle);
    OsStatus SysHandleClone(InvokeContext *context, OsHandle handle, OsHandleCloneInfo info, OsHandle *outHandle);
    OsStatus SysHandleStat(InvokeContext *context, OsHandle handle, OsHandleInfo *result);

    // node

    OsStatus SysNodeOpen(InvokeContext *context, NodeOpenInfo info, OsNodeHandle *outHandle);
    OsStatus SysNodeCreate(InvokeContext *context, sm::RcuSharedPtr<vfs2::INode> node, OsNodeHandle *outHandle);
    OsStatus SysNodeClose(InvokeContext *context, OsNodeHandle handle);
    OsStatus SysNodeQuery(InvokeContext *context, OsNodeHandle handle, OsNodeQueryInterfaceInfo info, OsDeviceHandle *outHandle);
    OsStatus SysNodeStat(InvokeContext *context, OsNodeHandle handle, OsNodeInfo *result);

    // device

    OsStatus SysDeviceOpen(InvokeContext *context, DeviceOpenInfo info, OsDeviceHandle *outHandle);
    OsStatus SysDeviceClose(InvokeContext *context, OsDeviceHandle handle);
    OsStatus SysDeviceRead(InvokeContext *context, OsDeviceHandle handle, OsDeviceReadRequest request, OsSize *outRead);
    OsStatus SysDeviceWrite(InvokeContext *context, OsDeviceHandle handle, OsDeviceWriteRequest request, OsSize *outWrite);
    OsStatus SysDeviceInvoke(InvokeContext *context, OsDeviceHandle handle, uint64_t function, void *data, size_t size);
    OsStatus SysDeviceStat(InvokeContext *context, OsDeviceHandle handle, OsDeviceInfo *info);

    // process

    OsStatus SysCreateProcess(InvokeContext *context, OsProcessCreateInfo info, OsProcessHandle *handle);
    OsStatus SysDestroyProcess(InvokeContext *context, OsProcessHandle handle, int64_t exitCode, OsProcessStateFlags reason);
    OsStatus SysProcessStat(InvokeContext *context, OsProcessHandle handle, OsProcessInfo *result);
    OsStatus SysProcessCurrent(InvokeContext *context, OsProcessAccess access, OsProcessHandle *handle);

    OsStatus SysCreateProcess(InvokeContext *context, sm::RcuSharedPtr<Process> parent, OsProcessCreateInfo info, OsProcessHandle *handle);
    OsStatus SysDestroyProcess(InvokeContext *context, ProcessHandle *handle, int64_t exitCode, OsProcessStateFlags reason);

    OsStatus SysQueryProcessList(InvokeContext *context, ProcessQueryInfo info, ProcessQueryResult *result);

    // vmem

    OsStatus SysCreateVmem(InvokeContext *context, OsVmemCreateInfo info, void **outVmem);
    OsStatus SysReleaseVmem(InvokeContext *context, VmemReleaseInfo info);
    OsStatus SysMapVmem(InvokeContext *context, VmemMapInfo info, void **outVmem);

    // thread

    OsStatus SysCreateThread(InvokeContext *context, ThreadCreateInfo info, OsThreadHandle *handle);
    OsStatus SysCreateThread(InvokeContext *context, OsThreadCreateInfo info, OsThreadHandle *handle);
    OsStatus SysDestroyThread(InvokeContext *context, OsThreadState reason, OsThreadHandle handle);
    OsStatus SysThreadStat(InvokeContext *context, OsThreadHandle handle, OsThreadInfo *result);

    // tx

    OsStatus SysCreateTx(InvokeContext *context, TxCreateInfo info, OsTxHandle *handle);
    OsStatus SysCommitTx(InvokeContext *context, OsTxHandle handle);
    OsStatus SysAbortTx(InvokeContext *context, OsTxHandle handle);

    // mutex

    OsStatus SysCreateMutex(InvokeContext *context, OsMutexCreateInfo info, OsHandle *handle);
    OsStatus SysDestroyMutex(InvokeContext *context, OsHandle handle);
}
