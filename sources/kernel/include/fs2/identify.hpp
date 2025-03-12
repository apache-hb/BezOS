#pragma once

#include <bezos/subsystem/identify.h>

#include "fs2/interface.hpp"

namespace vfs2 {
    template<typename T>
    concept IdentifyNode = requires (T it) {
        { it.identify(std::declval<OsIdentifyInfo*>()) } -> std::same_as<OsStatus>;
        { it.interfaces() } -> std::same_as<std::span<const OsGuid>>;
    };

    class IdentifyMixin {
    protected:
        OsIdentifyInfo mInfo;

    public:
        OsStatus identify(OsIdentifyInfo *info) {
            *info = mInfo;
            return OsStatusSuccess;
        }
    };

    template<IdentifyNode T>
    class TIdentifyHandle : public IIdentifyHandle {
        T *mNode;

    public:
        TIdentifyHandle(T *node)
            : mNode(node)
        { }

        OsStatus identify(OsIdentifyInfo *info) override {
            return mNode->identify(info);
        }

        OsStatus interfaces(OsIdentifyInterfaceList *list) override {
            std::span interfaces = mNode->interfaces();
            uint32_t count = std::min<uint32_t>(interfaces.size(), list->InterfaceCount);
            for (uint32_t i = 0; i < count; i++) {
                list->InterfaceGuids[i] = interfaces[i];
            }

            list->InterfaceCount = count;
            if (count < interfaces.size()) {
                return OsStatusMoreData;
            }

            return OsStatusSuccess;
        }

        HandleInfo info() override {
            return HandleInfo { mNode };
        }
    };
}
