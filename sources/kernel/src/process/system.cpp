#include "process/system.hpp"

#include "fs2/vfs.hpp"

#include "kernel.hpp"
#include "log.hpp"

using namespace km;

OsStatus SystemObjects::createVNode(const vfs2::VfsPath &path, sm::uuid uuid, const void *data, size_t size, Process *process, VNode **node) {
    std::unique_ptr<VNode> result { new (std::nothrow) VNode };
    if (result == nullptr) {
        return OsStatusOutOfMemory;
    }

    if (OsStatus status = mVfs->device(path, uuid, data, size, std::out_ptr(result->node))) {
        return status;
    }

    VNodeId id = mDeviceIds.allocate();

    result->init(id, stdx::String(path.name()), std::move(result->node));

    VNode *ptr = result.release();

    stdx::UniqueLock guard(mLock);
    process->addHandle(ptr);
    mNodes.insert({id, ptr});
    *node = ptr;

    return OsStatusSuccess;
}

OsStatus SystemObjects::destroyVNode(Process *process, VNode *node) {
    stdx::UniqueLock guard(mLock);

    OsHandle vnodeId = node->publicId();
    if (releaseHandle(node)) {
        process->removeHandle(vnodeId);
    }

    return OsStatusSuccess;
}

VNode *SystemObjects::getVNode(VNodeId id) {
    stdx::SharedLock guard(mLock);
    if (auto it = mNodes.find(id); it != mNodes.end()) {
        return it->second;
    }

    return nullptr;
}

OsStatus SystemObjects::addVNode(Process *process, std::unique_ptr<vfs2::IHandle> handle, VNode **node) {
    stdx::UniqueLock guard(mLock);

    vfs2::HandleInfo handleInfo = handle->info();
    vfs2::NodeInfo nodeInfo = handleInfo.node->info();

    VNode *result = new (std::nothrow) VNode;
    if (result == nullptr) {
        return OsStatusOutOfMemory;
    }

    VNodeId id = mDeviceIds.allocate();
    result->init(id, stdx::String(nodeInfo.name), std::move(handle));

    process->addHandle(result);

    mNodes.insert({id, result});
    *node = result;

    return OsStatusSuccess;
}

Thread *SystemObjects::createThread(stdx::String name, Process *process) {
    Thread *result = nullptr;
    if (OsStatus status = createThread(std::move(name), process, &result)) {
        KmDebugMessage("[PROC] Failed to create thread: ", status, "\n");
        return nullptr;
    }

    return result;
}

OsStatus SystemObjects::createProcess(stdx::String name, x64::Privilege privilege, MemoryRange pteMemory, Process **process) {
    SystemPageTables& systemTables = mMemory->pageTables();

    std::unique_ptr<Process> result{ new (std::nothrow) Process };
    if (result == nullptr) {
        return OsStatusOutOfMemory;
    }

    AddressMapping pteMapping{};
    if (OsStatus status = systemTables.map(pteMemory, PageFlags::eData, MemoryType::eWriteBack, &pteMapping)) {
        KmDebugMessage("[PROC] Failed to map process page tables: ", status, "\n");
        return status;
    }

    ProcessId id = mProcessIds.allocate();
    result->init(id, std::move(name), privilege, &systemTables, pteMapping, DefaultUserArea());

    Process *handle = result.release();

    stdx::UniqueLock guard(mLock);

    mProcesses.insert({id, handle});

    *process = handle;
    return OsStatusSuccess;
}

OsStatus SystemObjects::destroyProcess(Process *process) {
    stdx::UniqueLock guard(mLock);

    releaseHandle(process);

    return OsStatusSuccess;
}

Process *SystemObjects::getProcess(ProcessId id) {
    stdx::SharedLock guard(mLock);
    if (auto it = mProcesses.find(id); it != mProcesses.end()) {
        return it->second;
    }

    return nullptr;
}

OsStatus SystemObjects::exitProcess(Process *process, int64_t status) {
    OsProcessStateFlags state = (process->state.Status & ~eOsProcessStatusMask) | eOsProcessExited;
    process->state.Status = state;
    process->state.ExitCode = status;
    return OsStatusSuccess;
}

OsStatus SystemObjects::createThread(stdx::String name, Process *process, Thread **thread) {
    stdx::UniqueLock guard(mLock);

    std::unique_ptr<Thread> result{ new (std::nothrow) Thread };
    if (result == nullptr) {
        return OsStatusOutOfMemory;
    }

    AddressMapping kernelStack = mMemory->allocateStack(0x4000 * 4);
    ThreadId id = mThreadIds.allocate();
    result->init(id, std::move(name), process, kernelStack);

    Thread *ptr = result.release();

    process->addHandle(ptr);

    mThreads.insert({id, ptr});
    *thread = ptr;

    return OsStatusSuccess;
}

Mutex *SystemObjects::createMutex(stdx::String name) {
    MutexId id = mMutexIds.allocate();
    Mutex *ptr = new Mutex(id, std::move(name));

    stdx::UniqueLock guard(mLock);
    mMutexes.insert({id, ptr});
    return ptr;
}

Thread *SystemObjects::getThread(ThreadId id) {
    stdx::SharedLock guard(mLock);
    if (auto it = mThreads.find(id); it != mThreads.end()) {
        return it->second;
    }

    return nullptr;
}

Mutex *SystemObjects::getMutex(MutexId id) {
    stdx::SharedLock guard(mLock);
    if (auto it = mMutexes.find(id); it != mMutexes.end()) {
        return it->second;
    }

    return nullptr;
}

bool SystemObjects::releaseHandle(KernelObject *handle) {
    if (handle->releaseStrong()) {
        destroyHandle(handle);
        return true;
    }

    return false;
}

void SystemObjects::destroyHandle(KernelObject *object) {
    if (object->handleType() == eOsHandleProcess) {
        Process *process = static_cast<Process*>(object);
        for (auto& [_, handle] : process->handles) {
            releaseHandle(handle);
        }

        mProcesses.erase(ProcessId(object->internalId()));
    } else if (object->handleType() == eOsHandleThread) {
        mThreads.erase(ThreadId(object->internalId()));
    } else if (object->handleType() == eOsHandleDevice) {
        mNodes.erase(VNodeId(object->internalId()));
    }

    delete object;
}

KernelObject *SystemObjects::getHandle(OsHandle id) {
    // TODO: this is stupid, use generic handle lookup

    switch (OS_HANDLE_TYPE(id)) {
    case eOsHandleProcess:
        return getProcess(ProcessId(OS_HANDLE_ID(id)));
    case eOsHandleThread:
        return getThread(ThreadId(OS_HANDLE_ID(id)));
    case eOsHandleDevice:
        return getVNode(VNodeId(OS_HANDLE_ID(id)));
    case eOsHandleMutex:
        return getMutex(MutexId(OS_HANDLE_ID(id)));
    default:
        return nullptr;
    }
}
