#include "system/node.hpp"
#include "system/system.hpp"
#include "system/process.hpp"
#include "system/device.hpp"

#include "fs2/vfs.hpp"

static sys2::ObjectName GetVfsNodeName(sm::RcuSharedPtr<vfs2::INode> node) {
    auto info = node->info();
    return info.name;
}

sys2::Node::Node(sm::RcuSharedPtr<vfs2::INode> vfsNode)
    : BaseObject(GetVfsNodeName(vfsNode))
    , mVfsNode(vfsNode)
{ }

sys2::NodeHandle::NodeHandle(sm::RcuSharedPtr<Node> node, OsHandle handle, NodeAccess access)
    : BaseHandle(node, handle, access)
{ }

OsStatus sys2::SysNodeOpen(InvokeContext *context, NodeOpenInfo info, NodeHandle **handle) {
    vfs2::VfsRoot *root = context->system->mVfsRoot;
    ProcessHandle *parent = info.process;
    sm::RcuSharedPtr<vfs2::INode> vfsNode;

    if (!parent->hasAccess(ProcessAccess::eIoControl)) {
        return OsStatusAccessDenied;
    }

    if (OsStatus status = root->lookup(info.path, &vfsNode)) {
        return status;
    }

    sm::RcuSharedPtr<Process> process = parent->getProcess();

    sm::RcuSharedPtr<sys2::Node> node = sm::rcuMakeShared<sys2::Node>(&context->system->rcuDomain(), vfsNode);
    if (!node) {
        return OsStatusOutOfMemory;
    }

    NodeHandle *result = new (std::nothrow) NodeHandle(node, process->newHandleId(eOsHandleNode), NodeAccess::eAll);
    if (!result) {
        return OsStatusOutOfMemory;
    }

    process->addHandle(result);
    *handle = result;

    return OsStatusSuccess;
}

OsStatus sys2::SysNodeClose(InvokeContext *, NodeCloseInfo info) {
    ProcessHandle *parent = info.process;
    NodeHandle *node = info.node;

    if (!node->hasAccess(NodeAccess::eDestroy)) {
        return OsStatusAccessDenied;
    }

    if (!parent->hasAccess(ProcessAccess::eIoControl)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Process> process = parent->getProcess();
    if (OsStatus status = process->removeHandle(node)) {
        return status;
    }

    delete node;

    return OsStatusSuccess;
}

OsStatus sys2::SysNodeQuery(InvokeContext *context, NodeQueryInfo info, DeviceHandle **handle) {
    NodeHandle *node = info.node;
    std::unique_ptr<vfs2::IHandle> vfsHandle;
    ProcessHandle *hProcess = context->process;

    if (!hProcess->hasAccess(ProcessAccess::eIoControl)) {
        return OsStatusAccessDenied;
    }

    if (!node->hasAccess(NodeAccess::eQueryInterface)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Process> process = hProcess->getProcess();
    sm::RcuSharedPtr<Node> sysNode = node->getNode();
    sm::RcuSharedPtr<vfs2::INode> vfsNode = sysNode->getVfsNode();

    if (OsStatus status = vfsNode->query(info.uuid, info.data, info.size, std::out_ptr(vfsHandle))) {
        return status;
    }

    sm::RcuSharedPtr<sys2::Device> device = sm::rcuMakeShared<sys2::Device>(&context->system->rcuDomain(), std::move(vfsHandle));
    if (!device) {
        return OsStatusOutOfMemory;
    }

    DeviceHandle *result = new (std::nothrow) DeviceHandle(device, process->newHandleId(eOsHandleDevice), DeviceAccess::eAll);
    if (!result) {
        return OsStatusOutOfMemory;
    }

    process->addHandle(result);
    *handle = result;

    return OsStatusSuccess;
}

OsStatus sys2::SysNodeStat(InvokeContext *, NodeHandle *handle, NodeStat *result) {
    if (!handle->hasAccess(NodeAccess::eStat)) {
        return OsStatusAccessDenied;
    }

    sm::RcuSharedPtr<Node> node = handle->getNode();
    sm::RcuSharedPtr<vfs2::INode> vfsNode = node->getVfsNode();
    auto info = vfsNode->info();

    *result = NodeStat {
        .name = info.name,
    };

    return OsStatusSuccess;
}
