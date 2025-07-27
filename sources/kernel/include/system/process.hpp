#pragma once

#include <bezos/facility/process.h>
#include <bezos/facility/vmem.h>
#include <bezos/facility/tx.h>

#include "memory/range.hpp"

#include "system/base.hpp"
#include "system/handle.hpp"
#include "system/sanitize.hpp"
#include "system/thread.hpp"
#include "system/transaction.hpp"
#include "system/create.hpp"
#include "system/invoke.hpp"

#include "system/vmm.hpp"
#include "util/absl.hpp"

namespace vfs {
    class IFileHandle;
}

namespace sys {
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

        /// @brief All the handles this process has open.
        sm::FlatHashMap<OsHandle, std::unique_ptr<IHandle>> mHandles;

        AddressSpaceManager mAddressSpace;

        /// @brief Arguments passed to the process on creation.
        km::AddressMapping mInitArgsMapping;
        size_t mInitArgsSize;

        friend void SetProcessInitArgs(sys::System *system, sys::Process *process, std::span<std::byte> args);

    public:
        using Access = ProcessAccess;

        Process(
            ObjectName name,
            OsProcessStateFlags state,
            sm::RcuWeakPtr<Process> parent,
            OsProcessId pid,
            AddressSpaceManager&& addressSpace
        );

        stdx::StringView getClassName() const override { return "Process"; }

        OsStatus open(HandleCreateInfo createInfo, IHandle **handle) override;

        OsStatus stat(ProcessStat *info);
        bool isSupervisor() const { return mState & eOsProcessSupervisor; }

        IHandle *getHandle(OsHandle handle);
        void addHandle(IHandle *handle);
        OsStatus removeHandle(IHandle *handle);
        OsStatus removeHandle(OsHandle handle);
        OsStatus findHandle(OsHandle handle, OsHandleType type, IHandle **result);
        km::PhysicalAddress getPageMap() const;

        AddressSpaceManager *getAddressSpaceManager() { return &mAddressSpace; }

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

        OsStatus vmemCreate(System *system, VmemCreateInfo info, km::AddressMapping *mapping);
        OsStatus vmemMapFile(System *system, VmemMapInfo info, vfs::IFileHandle *fileHandle, km::VirtualRange *result);
        OsStatus vmemMapProcess(System *system, VmemMapInfo info, sm::RcuSharedPtr<Process> process, km::VirtualRange *result);
        OsStatus vmemMap(System *system, OsVmemMapInfo info, km::AddressMapping *mapping);
        OsStatus vmemRelease(System *system, km::VirtualRange range);

        void removeThread(sm::RcuSharedPtr<Thread> thread);
        void addThread(sm::RcuSharedPtr<Thread> thread);

        void loadPageTables();

        sm::RcuSharedPtr<Process> lockParent() { return mParent.lock(); }
    };

    class ProcessHandle final : public BaseHandle<Process> {
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
    [[nodiscard]]
    OsStatus SysFindHandle(InvokeContext *context, OsHandle handle, T **outHandle) {
        if (handle == OS_HANDLE_INVALID) {
            return OsStatusInvalidHandle;
        }

        IHandle *base = nullptr;
        if (OsStatus status = context->process->findHandle(handle, T::kHandleType, &base)) {
            return status;
        }

        *outHandle = static_cast<T*>(base);
        return OsStatusSuccess;
    }

    OsStatus SysGetInvokerTx(InvokeContext *context, TxHandle **outHandle);
}
