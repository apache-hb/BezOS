#pragma once

#include <bezos/facility/process.h>
#include <bezos/facility/vmem.h>
#include <bezos/facility/tx.h>

#include "memory/address_space.hpp"
#include "memory/range.hpp"
#include "std/vector.hpp"
#include "system/base.hpp"
#include "system/handle.hpp"
#include "system/thread.hpp"
#include "system/transaction.hpp"
#include "util/absl.hpp"

#include "system/create.hpp"

namespace sys2 {
    enum class ProcessId : uint64_t {};

    struct ProcessInfo {
        ObjectName name;
        uint64_t handles;
        bool supervisor;
        int64_t exitCode;
        OsProcessStateFlags state;
        ProcessId id;
        sm::RcuWeakPtr<Process> parent;
    };

    OsStatus CreateRootProcess(System *system, ProcessCreateInfo info, ProcessHandle **process);

    class Process final : public BaseObject {
        using Super = BaseObject;

        bool mSupervisor;
        ProcessId mId;
        OsProcessStateFlags mState;
        int64_t mExitCode;

        HandleAllocator<OsHandle> mIdAllocators[eOsHandleCount];

        sm::RcuWeakPtr<Process> mParent;

        sm::FlatHashSet<sm::RcuSharedPtr<Process>, sm::RcuHash<Process>, std::equal_to<>> mChildren;
        sm::FlatHashSet<sm::RcuSharedPtr<Thread>, sm::RcuHash<Thread>, std::equal_to<>> mThreads;

        /// @brief All the handles this process has open.
        sm::FlatHashMap<OsHandle, std::unique_ptr<IHandle>> mHandles;

        sm::BTreeMultiMap<sm::RcuWeakPtr<Process>, std::unique_ptr<IHandle>> mProcessHandles;

        /// @brief All the physical memory dedicated to this process.
        stdx::Vector2<km::MemoryRange> mPhysicalMemory;

        km::AddressMapping mPteMemory;

        /// @brief The page tables for this process.
        km::AddressSpace mPageTables;

    public:
        using Access = ProcessAccess;

        Process(const ProcessCreateInfo& createInfo, sm::RcuWeakPtr<Process> parent, const km::AddressSpace *systemTables, km::AddressMapping pteMemory);

        stdx::StringView getClassName() const override { return "Process"; }

        OsStatus stat(ProcessInfo *info);
        bool isSupervisor() const { return mSupervisor; }

        IHandle *getHandle(OsHandle handle);
        void addHandle(IHandle *handle);

        OsHandle newHandleId(OsHandleType type) {
            OsHandle id = mIdAllocators[type].allocate();
            return OS_HANDLE_NEW(type, id);
        }

        void removeChild(sm::RcuSharedPtr<Process> child);
        void addChild(sm::RcuSharedPtr<Process> child);

        OsStatus createProcess(System *system, ProcessCreateInfo info, ProcessHandle **handle);
        OsStatus destroy(System *system, const ProcessDestroyInfo& info);

        OsStatus createThread(System *system, ThreadCreateInfo info, ThreadHandle **handle);

        OsStatus createTx(System *system, TxCreateInfo info, TxHandle **handle);

        OsStatus vmemCreate(System *system, OsVmemCreateInfo info, km::AddressMapping *mapping);
        OsStatus vmemRelease(System *system, km::AddressMapping mapping);

        void removeThread(sm::RcuSharedPtr<Thread> thread);
        void addThread(sm::RcuSharedPtr<Thread> thread);
    };

    class ProcessHandle final : public BaseHandle<Process> {
        using Super = BaseHandle<Process>;

    public:
        ProcessHandle(sm::RcuSharedPtr<Process> process, OsHandle handle, ProcessAccess access)
            : Super(process, handle, access)
        { }

        sm::RcuSharedPtr<Process> getProcess() { return getInner(); }

        OsStatus createProcess(System *system, ProcessCreateInfo info, ProcessHandle **handle);
        OsStatus destroyProcess(System *system, const ProcessDestroyInfo& info);

        OsStatus createThread(System *system, ThreadCreateInfo info, ThreadHandle **handle);

        OsStatus createTx(System *system, TxCreateInfo info, TxHandle **handle);

        OsStatus stat(ProcessInfo *info);
    };
}
