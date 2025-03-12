#include "fs2/node.hpp"

using namespace vfs2;

OsStatus IVfsNodeHandle::read(ReadRequest request, ReadResult *result) {
    return node->read(request, result);
}

OsStatus IVfsNodeHandle::write(WriteRequest request, WriteResult *result) {
    return node->write(request, result);
}

OsStatus IVfsNodeHandle::stat(NodeStat *stat) {
    return node->stat(stat);
}

HandleInfo IVfsNodeHandle::info() {
    return HandleInfo { node };
}

VfsFolderHandle::VfsFolderHandle(IVfsNode *node)
    : IVfsNodeHandle(node)
{
    for (const auto& [name, _] : *node) {
        mEntries.add(VfsString(name));
    }
}

OsStatus VfsFolderHandle::next(VfsString *name) {
    if (mIndex >= mEntries.count()) {
        return OsStatusEndOfFile;
    }

    *name = mEntries[mIndex++];
    return OsStatusSuccess;
}
