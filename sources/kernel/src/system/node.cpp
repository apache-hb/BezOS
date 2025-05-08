#include "system/node.hpp"
#include "system/system.hpp"
#include "system/process.hpp"
#include "system/device.hpp"

#include "fs/vfs.hpp"

static sys::ObjectName GetVfsNodeName(sm::RcuSharedPtr<vfs::INode> node) {
    auto info = node->info();
    return info.name;
}

sys::Node::Node(sm::RcuSharedPtr<vfs::INode> vfsNode)
    : BaseObject(GetVfsNodeName(vfsNode))
    , mVfsNode(vfsNode)
{ }

sys::NodeHandle::NodeHandle(sm::RcuSharedPtr<Node> node, OsHandle handle, NodeAccess access)
    : BaseHandle(node, handle, access)
{ }

OsStatus sys::NodeHandle::clone(OsHandleAccess access, OsHandle id, IHandle **result) {
    if (!hasAccess(NodeAccess(access))) {
        return OsStatusAccessDenied;
    }

    NodeHandle *handle = new (std::nothrow) NodeHandle(getInner(), id, NodeAccess(access));
    if (!handle) {
        return OsStatusOutOfMemory;
    }

    *result = handle;
    return OsStatusSuccess;
}

OsStatus sys::SysNodeOpen(InvokeContext *context, NodeOpenInfo info, OsNodeHandle *outHandle) {
    vfs::VfsRoot *root = context->system->mVfsRoot;
    ProcessHandle *parent = info.process;
    sm::RcuSharedPtr<vfs::INode> vfsNode;

    if (!parent->hasAccess(ProcessAccess::eIoControl)) {
        return OsStatusAccessDenied;
    }

    if (OsStatus status = root->lookup(info.path, &vfsNode)) {
        return status;
    }

    sm::RcuSharedPtr<Process> process = parent->getProcess();

    sm::RcuSharedPtr<sys::Node> node = sm::rcuMakeShared<sys::Node>(&context->system->rcuDomain(), vfsNode);
    if (!node) {
        return OsStatusOutOfMemory;
    }

    NodeHandle *result = new (std::nothrow) NodeHandle(node, process->newHandleId(eOsHandleNode), NodeAccess::eAll);
    if (!result) {
        return OsStatusOutOfMemory;
    }

    process->addHandle(result);
    *outHandle = result->getHandle();

    return OsStatusSuccess;
}

OsStatus sys::SysNodeCreate(InvokeContext *context, sm::RcuSharedPtr<vfs::INode> vfsNode, OsNodeHandle *outHandle) {
    sm::RcuSharedPtr<sys::Node> node = sm::rcuMakeShared<sys::Node>(&context->system->rcuDomain(), vfsNode);
    if (!node) {
        return OsStatusOutOfMemory;
    }

    NodeHandle *result = new (std::nothrow) NodeHandle(node, context->process->newHandleId(eOsHandleNode), NodeAccess::eAll);
    if (!result) {
        return OsStatusOutOfMemory;
    }

    context->process->addHandle(result);
    *outHandle = result->getHandle();

    return OsStatusSuccess;
}

OsStatus sys::SysNodeClose(InvokeContext *context, OsNodeHandle handle) {
    NodeHandle *node = nullptr;
    if (OsStatus status = SysFindHandle(context, handle, &node)) {
        return status;
    }

    if (!node->hasAccess(NodeAccess::eDestroy)) {
        return OsStatusAccessDenied;
    }

    if (OsStatus status = context->process->removeHandle(node)) {
        return status;
    }

    return OsStatusSuccess;
}

OsStatus sys::SysNodeQuery(InvokeContext *context, OsNodeHandle handle, OsNodeQueryInterfaceInfo info, OsDeviceHandle *outHandle) {
    NodeHandle *node = nullptr;
    std::unique_ptr<vfs::IHandle> vfsHandle;

    if (OsStatus status = SysFindHandle(context, handle, &node)) {
        return status;
    }

    if (!node->hasAccess(NodeAccess::eQueryInterface)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Node> sysNode = node->getNode();
    sm::RcuSharedPtr<vfs::INode> vfsNode = sysNode->getVfsNode();

    if (OsStatus status = vfsNode->query(info.InterfaceGuid, info.OpenData, info.OpenDataSize, std::out_ptr(vfsHandle))) {
        return status;
    }

    sm::RcuSharedPtr<sys::Device> device = sm::rcuMakeShared<sys::Device>(&context->system->rcuDomain(), std::move(vfsHandle));
    if (!device) {
        return OsStatusOutOfMemory;
    }

    DeviceHandle *result = new (std::nothrow) DeviceHandle(device, context->process->newHandleId(eOsHandleDevice), DeviceAccess::eAll);
    if (!result) {
        return OsStatusOutOfMemory;
    }

    context->process->addHandle(result);
    *outHandle = result->getHandle();

    return OsStatusSuccess;
}

OsStatus sys::SysNodeStat(InvokeContext *context, OsNodeHandle handle, OsNodeInfo *result) {
    NodeHandle *node = nullptr;
    if (OsStatus status = SysFindHandle(context, handle, &node)) {
        return status;
    }

    if (!node->hasAccess(NodeAccess::eStat)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Node> sysNode = node->getNode();
    sm::RcuSharedPtr<vfs::INode> vfsNode = sysNode->getVfsNode();
    auto info = vfsNode->info();

    OsNodeInfo nodeInfo{};
    size_t size = std::min(sizeof(nodeInfo.Name), info.name.count());
    std::memcpy(nodeInfo.Name, info.name.data(), size);

    *result = nodeInfo;

    return OsStatusSuccess;
}
