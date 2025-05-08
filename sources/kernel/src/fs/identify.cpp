#include "fs/identify.hpp"

OsStatus vfs::detail::ValidateInterfaceList(void *data, size_t size) {
    //
    // Reject input buffers that are too small to contain the header.
    //
    if ((data == nullptr) || (size < sizeof(OsIdentifyInterfaceList))) {
        return OsStatusInvalidInput;
    }

    //
    // Reject input buffers that have a suspicious size.
    //
    size_t bodySize = size - sizeof(OsIdentifyInterfaceList);
    if (bodySize % sizeof(OsGuid) != 0) {
        return OsStatusInvalidInput;
    }

    //
    // If the header doesnt report the correct size, reject the input.
    //
    OsIdentifyInterfaceList *list = static_cast<OsIdentifyInterfaceList*>(data);
    if (list->InterfaceCount != (bodySize / sizeof(OsGuid))) {
        return OsStatusInvalidInput;
    }

    return OsStatusSuccess;
}

OsStatus vfs::IIdentifyHandle::identify(void *data, size_t size) {
    if (data == nullptr || size != sizeof(OsIdentifyInfo)) {
        return OsStatusInvalidInput;
    }

    return identify(static_cast<OsIdentifyInfo*>(data));
}

OsStatus vfs::IIdentifyHandle::interfaces(void *data, size_t size) {
    if (OsStatus status = detail::ValidateInterfaceList(data, size)) {
        return status;
    }

    OsIdentifyInterfaceList *list = static_cast<OsIdentifyInterfaceList*>(data);
    return interfaces(list);
}

OsStatus vfs::IIdentifyHandle::invoke(IInvokeContext *, uint64_t function, void *data, size_t size) {
    switch (function) {
    case eOsIdentifyInfo:
        return identify(data, size);
    case eOsIdentifyInterfaceList:
        return interfaces(data, size);

    default:
        return OsStatusFunctionNotSupported;
    }
}
