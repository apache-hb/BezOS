#pragma once

#include <bezos/subsystem/identify.h>

#include "fs2/interface.hpp"

namespace vfs2 {
    namespace detail {
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

        OsStatus identify(OsIdentifyInfo *info) override {
            return mNode->identify(info);
        }

        OsStatus interfaces(OsIdentifyInterfaceList *list) override {
            return mNode->interfaces(list);
        }

        HandleInfo info() override {
            return HandleInfo { mNode };
        }
    };
}
