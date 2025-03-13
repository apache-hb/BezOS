#pragma once

#include <bezos/subsystem/identify.h>

#include "fs2/interface.hpp"

namespace vfs2 {
    namespace detail {
        OsStatus InterfaceList(void *data, size_t size, std::span<const OsGuid> interfaces);
    }

    template<typename T>
    concept IdentifyNode = requires (T it) {
        { it.identify(std::declval<OsIdentifyInfo*>()) } -> std::same_as<OsStatus>;
    };

    template<typename T>
    concept IdentifyInterfaceList = requires (T it) {
        { it.interfaces() } -> std::same_as<std::span<const OsGuid>>;
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
            if (size != sizeof(OsIdentifyInfo)) {
                return OsStatusInvalidInput;
            }

            return mNode->identify(static_cast<OsIdentifyInfo*>(data));
        }

        OsStatus interfaces(void *data, size_t size) override {
            if constexpr (IdentifyInterfaceList<T>) {
                //
                // Forward implementation to detail function to reduce code size.
                //
                return detail::InterfaceList(data, size, mNode->interfaces());
            } else {
                //
                // The class provides its own interfaces method.
                //
                return mNode->interfaces(data, size);
            }
        }

        HandleInfo info() override {
            return HandleInfo { mNode };
        }
    };
}
