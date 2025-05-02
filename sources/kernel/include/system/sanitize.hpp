#pragma once

#include <bezos/status.h>
#include <bezos/facility/vmem.h>

#include "common/virtual_address.hpp"
#include "memory/memory.hpp"

namespace sys {
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
    };

    template<>
    struct Sanitize<VmemCreateInfo> {
        using ApiObject = OsVmemCreateInfo;
        using KernelObject = VmemCreateInfo;

        static OsStatus sanitize(const ApiObject *info, KernelObject *result) noexcept;
    };
}
