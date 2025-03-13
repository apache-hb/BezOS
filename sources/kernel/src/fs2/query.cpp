#include "fs2/query.hpp"

#include <bezos/subsystem/identify.h>

OsStatus vfs2::detail::ServiceInterfaceList(std::span<const InterfaceEntry> interfaces, OsIdentifyInterfaceList *list) {
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
