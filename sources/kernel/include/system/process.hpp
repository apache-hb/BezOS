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
#include "system/create.hpp"
#include "system/invoke.hpp"

#include "util/absl.hpp"

namespace sys2 {
    enum class ProcessId : OsProcessId {};

    class Process final : public BaseObject<eOsHandleProcess> {
        using Super = BaseObject<eOsHandleProcess>;

        ProcessId mId;
        OsProcessStateFlags mState;
        int64_t mExitCode;

        HandleAllocator<OsHandle> mIdAllocators[eOsHandleCount];

        sm::RcuWeakPtr<Process> mParent;

        sm::FlatHashSet<sm::RcuSharedPtr<Process>, sm::RcuHash<Process>, std::equal_to<>> mChildren;
        sm::FlatHashSet<sm::RcuSharedPtr<Thread>, sm::RcuHash<Thread>, std::equal_to<>> mThreads;

        sm::FlatHashMap<sm::RcuDynamicPtr<IObject>, OsHandle, sm::RcuHash<IObject>, std::equal_to<>> mObjectHandles;

        /// @brief All the handles this process has open.
        sm::FlatHashMap<OsHandle, std::unique_ptr<IHandle>> mHandles;

        sm::BTreeMultiMap<sm::RcuDynamicPtr<Process>, std::unique_ptr<IHandle>> mProcessHandles;

        /// @brief All the physical memory dedicated to this process.
        stdx::Vector2<km::MemoryRange> mPhysicalMemory;

        km::AddressMapping mPteMemory;

        /// @brief The page tables for this process.
        km::AddressSpace mPageTables;

    public:
        using Access = ProcessAccess;

        Process(ObjectName name, OsProcessStateFlags state, sm::RcuWeakPtr<Process> parent, OsProcessId pid, const km::AddressSpace *systemTables, km::AddressMapping pteMemory);

        stdx::StringView getClassName() const override { return "Process"; }

        OsStatus open(HandleCreateInfo createInfo, IHandle **handle) override;

        OsStatus stat(ProcessStat *info);
        bool isSupervisor() const { return mState & eOsProcessSupervisor; }

        IHandle *getHandle(OsHandle handle);
        void addHandle(IHandle *handle);
        OsStatus removeHandle(IHandle *handle);
        OsStatus removeHandle(OsHandle handle);
        OsStatus findHandle(OsHandle handle, OsHandleType type, IHandle **result);

        OsStatus currentHandle(OsProcessAccess access, OsProcessHandle *handle);

        OsStatus resolveObject(sm::RcuSharedPtr<IObject> object, OsHandleAccess access, OsHandle *handle);

        template<typename T>
        OsStatus findHandle(OsHandle handle, T **result) {
            IHandle *base = nullptr;
            if (OsStatus status = findHandle(handle, T::kHandleType, &base)) {
                return status;
            }

            *result = static_cast<T *>(base);
            return OsStatusSuccess;
        }

        OsHandle newHandleId(OsHandleType type) {
            OsHandle id = mIdAllocators[type].allocate();
            return OS_HANDLE_NEW(type, id);
        }

        void removeChild(sm::RcuSharedPtr<Process> child);
        void addChild(sm::RcuSharedPtr<Process> child);

        OsStatus createProcess(System *system, ProcessCreateInfo info, ProcessHandle **handle);
        OsStatus destroy(System *system, const ProcessDestroyInfo& info);

        OsStatus createTx(System *system, TxCreateInfo info, TxHandle **handle);

        OsStatus vmemCreate(System *system, OsVmemCreateInfo info, km::AddressMapping *mapping);
        OsStatus vmemMap(System *system, OsVmemMapInfo info, km::AddressMapping *mapping);
        OsStatus vmemRelease(System *system, km::VirtualRange range);

        void removeThread(sm::RcuSharedPtr<Thread> thread);
        void addThread(sm::RcuSharedPtr<Thread> thread);

        sm::RcuSharedPtr<Process> lockParent() { return mParent.lock(); }
    };

    class ProcessHandle final : public BaseHandle<Process, eOsHandleProcess> {
    public:
        ProcessHandle(sm::RcuSharedPtr<Process> process, OsHandle handle, ProcessAccess access)
            : BaseHandle(process, handle, access)
        { }

        sm::RcuSharedPtr<Process> getProcess() { return getInner(); }

        OsStatus createProcess(System *system, ProcessCreateInfo info, ProcessHandle **handle);
        OsStatus destroyProcess(System *system, const ProcessDestroyInfo& info);
    };

    bool IsParentProcess(sm::RcuSharedPtr<Process> process, sm::RcuSharedPtr<Process> parent);

    template<typename T>
    OsStatus SysFindHandle(InvokeContext *context, OsHandle handle, T **outHandle) {
        if (handle == OS_HANDLE_INVALID) {
            return OsStatusInvalidHandle;
        }

        T *result = nullptr;
        if (OsStatus status = context->process->findHandle(handle, &result)) {
            return status;
        }

        *outHandle = result;
        return OsStatusSuccess;
    }

    OsStatus SysGetInvokerTx(InvokeContext *context, TxHandle **outHandle);
}
