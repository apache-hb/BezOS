#include "process/process.hpp"

#include "process/device.hpp"

using namespace km;

void Process::init(ProcessId id, stdx::String name, SystemPageTables *kernel, AddressMapping pteMemory, VirtualRange processArea, ProcessCreateInfo createInfo) {
    initHeader(std::to_underlying(id), eOsHandleProcess, std::move(name));
    ptes.reset(new ProcessPageTables(kernel, pteMemory, processArea));
    parent = createInfo.parent;
}

bool Process::isComplete() const {
    auto status = OS_PROCESS_STATUS(state);
    return status != eOsProcessRunning && status != eOsProcessSuspended;
}

void Process::terminate(OsProcessStateFlags state, int64_t exitCode) {
    this->state = state;
    this->exitCode = exitCode;
}

void Process::addHandle(KernelObject *object) {
    handles.insert({ object->publicId(), object });
}

KernelObject *Process::findHandle(OsHandle id) {
    if (auto it = handles.find(id); it != handles.end()) {
        return it->second;
    }

    return nullptr;
}

OsStatus Process::removeHandle(OsHandle id) {
    if (auto it = handles.find(id); it != handles.end()) {
        handles.erase(it);
        return OsStatusSuccess;
    }

    return OsStatusNotFound;
}

OsStatus Process::map(km::SystemMemory& memory, size_t pages, PageFlags flags, MemoryType type, AddressMapping *mapping) {
    MemoryRange range = pmm.allocate(pages);
    if (range.isEmpty()) {
        range = memory.pmmAllocate(pages);
    }

    if (range.isEmpty()) {
        return OsStatusOutOfMemory;
    }

    OsStatus status = ptes->map(range, flags, type, mapping);
    if (status != OsStatusSuccess) {
        pmm.release(range);
    }

    return status;
}

OsStatus Process::createTls(km::SystemMemory& memory, Thread *thread) {
    return tlsInit.createTls(memory, thread);
}

void Node::init(NodeId id, stdx::String name, vfs2::INode *vfsNode) {
    initHeader(std::to_underlying(id), eOsHandleNode, std::move(name));
    node = vfsNode;
}

void Device::init(DeviceId id, stdx::String name, std::unique_ptr<vfs2::IHandle> vfsHandle) {
    initHeader(std::to_underlying(id), eOsHandleDevice, std::move(name));
    handle = std::move(vfsHandle);
}
