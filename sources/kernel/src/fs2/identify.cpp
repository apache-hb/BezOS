#include "fs2/identify.hpp"

OsStatus vfs2::detail::ValidateInterfaceList(void *data, size_t size) {
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

OsStatus vfs2::detail::InterfaceList(void *data, size_t size, std::span<const OsGuid> interfaces) {
    //
    // Reject any input buffers that are too small to contain the header.
    //
    if (size < sizeof(OsIdentifyInterfaceList)) {
        return OsStatusInvalidInput;
    }

    //
    // Reject any input that has a suspicious size.
    // * Any buffer that is not a multiple of the size of a GUID is invalid.
    // * Any buffer thats reported size is less than the size of the data structure is invalid.
    //
    OsIdentifyInterfaceList *list = reinterpret_cast<OsIdentifyInterfaceList*>(data);
    size_t listSize = (size - sizeof(OsIdentifyInterfaceList));
    if ((listSize % sizeof(OsGuid) != 0) || (listSize / sizeof(OsGuid) < list->InterfaceCount)) {
        return OsStatusInvalidInput;
    }

    //
    // Copy the interface list into the buffer.
    //
    uint32_t count = std::min<uint32_t>(interfaces.size(), list->InterfaceCount);
    for (uint32_t i = 0; i < count; i++) {
        list->InterfaceGuids[i] = interfaces[i];
    }

    //
    // If there are more interfaces than the buffer can hold report this back to the caller.
    //
    list->InterfaceCount = count;
    if (count < interfaces.size()) {
        return OsStatusMoreData;
    }

    return OsStatusSuccess;
}

OsStatus vfs2::IIdentifyHandle::invoke(uint64_t function, void *data, size_t size) {
    switch (function) {
    case eOsIdentifyInfo:
        return identify(data, size);
    case eOsIdentifyInterfaceList:
        return interfaces(data, size);

    default:
        return OsStatusFunctionNotSupported;
    }
}
