#include "fs2/interface.hpp"

OsStatus vfs2::IIteratorHandle::next(void *data, size_t size) {
    if ((data == nullptr) || (size != sizeof(OsIteratorNext))) {
        return OsStatusInvalidInput;
    }

    OsIteratorNext *iterator = static_cast<OsIteratorNext*>(data);
    INode *node = nullptr;
    if (OsStatus status = next(&node)) {
        return status;
    }

    // TODO: invoke needs to be aware of the system context
    iterator->Node = OS_HANDLE_INVALID; // node->handle();
    return OsStatusSuccess;
}

OsStatus vfs2::IIteratorHandle::invoke(uint64_t function, void *data, size_t size) {
    switch (function) {
    case eOsIteratorNext:
        return next(data, size);

    default:
        return OsStatusFunctionNotSupported;
    }
}
