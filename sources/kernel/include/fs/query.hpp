#pragma once

#include "std/rcuptr.hpp"
#include "util/uuid.hpp"

#include <new>
#include <array>
#include <span>

struct OsIdentifyInterfaceList;

namespace vfs {
    class IHandle;
    class INode;

    using CreateHandle = IHandle *(*)(sm::RcuSharedPtr<INode>, const void *, size_t);

    struct InterfaceEntry {
        OsGuid uuid;
        CreateHandle create;
    };

    template<typename THandle, typename TNode>
    constexpr InterfaceEntry InterfaceOf(OsGuid guid) {
        CreateHandle callback = [](sm::RcuSharedPtr<INode> node, const void *data, size_t size) -> IHandle* {
            return new (std::nothrow) THandle(sm::rcuSharedPtrCast<TNode>(node), data, size);
        };

        return InterfaceEntry { guid, callback, };
    }

    template<size_t N>
    class InterfaceList {
        std::array<InterfaceEntry, N> mInterfaces;
    public:
        constexpr InterfaceList(std::array<InterfaceEntry, N> interfaces)
            : mInterfaces(interfaces)
        { }

        OsStatus list(OsIdentifyInterfaceList *list) const;
        OsStatus query(sm::RcuSharedPtr<INode> node, sm::uuid uuid, const void *data, size_t size, IHandle **handle) const;
    };

    namespace detail {
        OsStatus ServiceInterfaceList(std::span<const InterfaceEntry> interfaces, OsIdentifyInterfaceList *list);
        OsStatus ServiceQuery(std::span<const InterfaceEntry> interfaces, sm::RcuSharedPtr<INode> node, sm::uuid uuid, const void *data, size_t size, IHandle **handle);
    }
}

template<size_t N>
OsStatus vfs::InterfaceList<N>::list(OsIdentifyInterfaceList *list) const {
    return detail::ServiceInterfaceList(mInterfaces, list);
}

template<size_t N>
OsStatus vfs::InterfaceList<N>::query(sm::RcuSharedPtr<INode> node, sm::uuid uuid, const void *data, size_t size, IHandle **handle) const {
    return detail::ServiceQuery(mInterfaces, node, uuid, data, size, handle);
}
