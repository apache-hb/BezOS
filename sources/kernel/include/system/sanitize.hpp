#pragma once

#include <bezos/status.h>
#include <bezos/facility/vmem.h>

#include "common/virtual_address.hpp"
#include "memory/memory.hpp"
#include "std/rcuptr.hpp"

namespace sys {
    class IObject;

    template<typename T>
    struct Sanitize {
        using ApiObject = void;
        using KernelObject = void;

        [[gnu::error("Sanitize<T>::sanitize() must be specialized")]]
        static OsStatus sanitize(const ApiObject *info, KernelObject *result) noexcept;
    };

    struct VmemCreateInfo {
        size_t size;
        size_t alignment;
        sm::VirtualAddress baseAddress;
        km::PageFlags flags;
        bool zeroMemory;
        bool commit, reserve;
        bool addressIsHint;
    };

    template<>
    struct Sanitize<VmemCreateInfo> {
        using ApiObject = OsVmemCreateInfo;
        using KernelObject = VmemCreateInfo;

        static OsStatus sanitize(const ApiObject *info [[gnu::nonnull]], KernelObject *result [[gnu::nonnull]]) noexcept;
    };

    struct VmemMapInfo {
        size_t size;
        size_t alignment;
        sm::VirtualAddress baseAddress;
        km::PageFlags flags;
        sm::VirtualAddress srcAddress;
        sm::RcuSharedPtr<IObject> srcObject;
    };

    template<>
    struct Sanitize<VmemMapInfo> {
        using ApiObject = OsVmemMapInfo;
        using KernelObject = VmemMapInfo;

        static OsStatus sanitize(const ApiObject *info [[gnu::nonnull]], KernelObject *result [[gnu::nonnull]]) noexcept;
    };
}
