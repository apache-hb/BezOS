#include "fs2/vfs.hpp"

using namespace vfs2;

OsStatus IVfsNodeHandle::read(ReadRequest request, ReadResult *result) {
    return node->read(request, result);
}

OsStatus IVfsNodeHandle::write(WriteRequest request, WriteResult *result) {
    return node->write(request, result);
}
