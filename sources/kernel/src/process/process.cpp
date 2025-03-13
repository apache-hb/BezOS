#include "process/process.hpp"

#include "util/defer.hpp"

using namespace km;

void Process::init(ProcessId id, stdx::String name, x64::Privilege protection, SystemPageTables *kernel, AddressMapping pteMemory, VirtualRange processArea) {
    initHeader(std::to_underlying(id), eOsHandleProcess, std::move(name));
    privilege = protection;
    ptes.init(kernel, pteMemory, processArea);
}

bool Process::isComplete() const {
    auto status = OS_PROCESS_STATUS(state.Status);
    return status != eOsProcessRunning && status != eOsProcessSuspended;
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

void Node::init(NodeId id, stdx::String name, vfs2::INode *vfsNode) {
    initHeader(std::to_underlying(id), eOsHandleNode, std::move(name));
    node = vfsNode;
}

void Device::init(DeviceId id, stdx::String name, std::unique_ptr<vfs2::IHandle> vfsHandle) {
    initHeader(std::to_underlying(id), eOsHandleDevice, std::move(name));
    handle = std::move(vfsHandle);
}
