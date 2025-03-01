#include "fs2/node.hpp"

using namespace vfs2;

OsStatus IVfsNodeHandle::read(ReadRequest request, ReadResult *result) {
    return node->read(request, result);
}

OsStatus IVfsNodeHandle::write(WriteRequest request, WriteResult *result) {
    return node->write(request, result);
}

OsStatus IVfsNodeHandle::stat(VfsNodeStat *stat) {
    return node->stat(stat);
}

VfsFolderHandle::VfsFolderHandle(IVfsNode *node)
    : IVfsNodeHandle(node)
{
    for (const auto& [name, _] : node->children) {
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
