#pragma once

#include <bezos/subsystem/identify.h>

#include "fs2/interface.hpp"

namespace vfs2 {
    namespace detail {
        OsStatus InterfaceList(void *data, size_t size, std::span<const OsGuid> interfaces);
        OsStatus ValidateInterfaceList(void *data, size_t size);
    }

    template<typename T>
    concept IdentifyNode = requires (T it) {
        { it.identify(std::declval<OsIdentifyInfo*>()) } -> std::same_as<OsStatus>;
        { it.interfaces(std::declval<OsIdentifyInterfaceList*>()) } -> std::same_as<OsStatus>;
    };

    template<const OsIdentifyInfo& Info>
    class ConstIdentifyMixin {
    public:
        OsStatus identify(OsIdentifyInfo *info) {
            *info = Info;
            return OsStatusSuccess;
        }
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

        OsStatus identify(void *data, size_t size) override {
            if (data == nullptr || size != sizeof(OsIdentifyInfo)) {
                return OsStatusInvalidInput;
            }

            return mNode->identify(static_cast<OsIdentifyInfo*>(data));
        }

        OsStatus interfaces(void *data, size_t size) override {
            if (OsStatus status = detail::ValidateInterfaceList(data, size)) {
                return status;
            }

            OsIdentifyInterfaceList *list = static_cast<OsIdentifyInterfaceList*>(data);
            return mNode->interfaces(list);
        }

        HandleInfo info() override {
            return HandleInfo { mNode };
        }
    };
}
