#pragma once

#include <bezos/status.h>
#include <bezos/facility/vmem.h>

#include "common/virtual_address.hpp"
#include "memory/memory.hpp"
#include "std/rcuptr.hpp"

namespace sys {
    class IObject;
    class Process;
    class InvokeContext;

    template<typename T>
    struct Sanitize {
        using ApiObject = void;
        using KernelObject = void;

        [[gnu::error("Sanitize<T>::sanitize() must be specialized")]]
        static OsStatus sanitize(InvokeContext *context, const ApiObject *info, KernelObject *result) noexcept;
    };

    enum class VmemCreateMode {
        eCommit,
        eReserve,
    };

    struct VmemCreateInfo {
        size_t size;
        size_t alignment;
        sm::VirtualAddress baseAddress;
        km::PageFlags flags;
        bool zeroMemory;
        VmemCreateMode mode;
        bool addressIsHint;
        sm::RcuSharedPtr<Process> process;
    };

    template<>
    struct Sanitize<VmemCreateInfo> {
        using ApiObject = OsVmemCreateInfo;
        using KernelObject = VmemCreateInfo;

        static OsStatus sanitize(InvokeContext *context, const ApiObject *info [[gnu::nonnull]], KernelObject *result [[gnu::nonnull]]) noexcept;
    };

    struct VmemMapInfo {
        size_t size;
        sm::VirtualAddress baseAddress;
        km::PageFlags flags;
        sm::VirtualAddress srcAddress;
        sm::RcuSharedPtr<IObject> srcObject;
        sm::RcuSharedPtr<Process> process;
    };

    template<>
    struct Sanitize<VmemMapInfo> {
        using ApiObject = OsVmemMapInfo;
        using KernelObject = VmemMapInfo;

        static OsStatus sanitize(InvokeContext *context, const ApiObject *info [[gnu::nonnull]], KernelObject *result [[gnu::nonnull]]) noexcept;
    };

    template<typename T>
    OsStatus sanitize(InvokeContext *context, const typename Sanitize<T>::ApiObject *info [[gnu::nonnull]], T *result [[gnu::nonnull]]) noexcept {
        return Sanitize<T>::sanitize(context, info, result);
    }
}

template<>
struct km::Format<sys::VmemCreateMode> {
    constexpr static void format(km::IOutStream& out, sys::VmemCreateMode value) noexcept {
        switch (value) {
        case sys::VmemCreateMode::eCommit:
            out.write("Commit");
            break;
        case sys::VmemCreateMode::eReserve:
            out.write("Reserve");
            break;
        }
    }
};
