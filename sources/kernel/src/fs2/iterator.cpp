#include "fs2/interface.hpp"

OsStatus vfs2::IIteratorHandle::next(IInvokeContext *context, void *data, size_t size) {
    if ((data == nullptr) || (size != sizeof(OsIteratorNext))) {
        return OsStatusInvalidInput;
    }

    OsIteratorNext *iterator = static_cast<OsIteratorNext*>(data);
    sm::RcuSharedPtr<INode> node = nullptr;
    if (OsStatus status = next(&node)) {
        return status;
    }

    iterator->Node = context->resolveNode(node);
    return OsStatusSuccess;
}

OsStatus vfs2::IIteratorHandle::invoke(IInvokeContext *context, uint64_t function, void *data, size_t size) {
    switch (function) {
    case eOsIteratorNext:
        return next(context, data, size);

    default:
        return OsStatusFunctionNotSupported;
    }
}
