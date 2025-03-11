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

vfs2::IVfsNodeHandle *Process::addFile(std::unique_ptr<vfs2::IVfsNodeHandle> handle) {
    stdx::UniqueLock guard(lock);
    vfs2::IVfsNodeHandle *ptr = handle.get();
    files.insert(std::move(handle));
    return ptr;
}

OsStatus Process::closeFile(vfs2::IVfsNodeHandle *ptr) {
    stdx::UniqueLock guard(lock);
    // This is awful, but theres no transparent lookup for unique pointers
    std::unique_ptr<vfs2::IVfsNodeHandle> handle(const_cast<vfs2::IVfsNodeHandle*>(ptr));
    defer { (void)handle.release(); }; // NOLINT(bugprone-unused-return-value)

    if (auto it = files.find(handle); it != files.end()) {
        files.erase(it);
        return OsStatusSuccess;
    }

    return OsStatusNotFound;
}

vfs2::IVfsNodeHandle *Process::findFile(const vfs2::IVfsNodeHandle *ptr) {
    // This is awful, but theres no transparent lookup for unique pointers
    std::unique_ptr<vfs2::IVfsNodeHandle> handle(const_cast<vfs2::IVfsNodeHandle*>(ptr));
    defer { (void)handle.release(); }; // NOLINT(bugprone-unused-return-value)

    stdx::SharedLock guard(lock);
    if (auto it = files.find(handle); it != files.end()) {
        return it->get();
    }

    return nullptr;
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

void VNode::init(VNodeId id, stdx::String name, std::unique_ptr<vfs2::IVfsNodeHandle> handle) {
    initHeader(std::to_underlying(id), eOsHandleDevice, std::move(name));
    node = std::move(handle);
}
