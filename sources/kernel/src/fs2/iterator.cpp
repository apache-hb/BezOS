#include "fs2/interface.hpp"

#include "process/system.hpp"

OsStatus vfs2::IIteratorHandle::next(km::SystemObjects *context, void *data, size_t size) {
    if ((data == nullptr) || (size != sizeof(OsIteratorNext))) {
        return OsStatusInvalidInput;
    }

    OsIteratorNext *iterator = static_cast<OsIteratorNext*>(data);
    INode *node = nullptr;
    if (OsStatus status = next(&node)) {
        return status;
    }

    iterator->Node = context->getNodeId(node);
    return OsStatusSuccess;
}

OsStatus vfs2::IIteratorHandle::invoke(km::SystemObjects *context, uint64_t function, void *data, size_t size) {
    switch (function) {
    case eOsIteratorNext:
        return next(context, data, size);

    default:
        return OsStatusFunctionNotSupported;
    }
}
