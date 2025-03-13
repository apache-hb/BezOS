#include "fs2/query.hpp"

#include <bezos/subsystem/identify.h>

OsStatus vfs2::detail::ServiceInterfaceList(std::span<const InterfaceEntry> interfaces, void *data, size_t size) {
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
        list->InterfaceGuids[i] = interfaces[i].uuid;
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

OsStatus vfs2::detail::ServiceQuery(std::span<const InterfaceEntry> interfaces, INode *node, sm::uuid uuid, const void *data, size_t size, IHandle **handle) {
    for (const auto& [iid, create] : interfaces) {
        if (iid == uuid) {
            IHandle *result = create(node, data, size);
            if (result == nullptr) {
                return OsStatusOutOfMemory;
            }

            *handle = result;
            return OsStatusSuccess;
        }
    }

    return OsStatusInterfaceNotSupported;
}
