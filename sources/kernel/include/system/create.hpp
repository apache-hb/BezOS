#pragma once

#include "system/access.hpp" // IWYU pragma: export

#include "fs/path.hpp"

#include "arch/intrin.hpp"

#include "std/rcuptr.hpp"
#include "std/static_string.hpp"
#include "util/absl.hpp"
#include "util/uuid.hpp"

namespace sys {
    using ObjectName = stdx::StaticString<OS_OBJECT_NAME_MAX>;

    using reg_t = uint64_t;

    using RegisterSet = arch::Intrin::MachineState;

    // handle

    struct HandleCloseInfo {
        ProcessHandle *process;
        IHandle *handle;
    };

    struct HandleCloneInfo {
        ProcessHandle *process;
        IHandle *handle;
        OsHandleAccess access;
    };

    struct HandleStat {
        OsHandleAccess access;
    };

    // tx

    struct TxCreateInfo {
        ObjectName name;
    };

    // node

    struct NodeOpenInfo {
        ProcessHandle *process;

        vfs::VfsPath path;
    };

    struct NodeCloseInfo {
        ProcessHandle *process;

        NodeHandle *node;
    };

    struct NodeQueryInfo {
        NodeHandle *node;
        sm::uuid uuid;
        void *data;
        size_t size;
    };

    struct NodeStat {
        ObjectName name;
    };

    struct DeviceOpenInfo {
        vfs::VfsPath path;
        OsDeviceCreateFlags flags;
        sm::uuid interface;
        void *data;
        size_t size;
    };

    struct DeviceCloseInfo {

    };

    // process

    struct ProcessCreateInfo {
        ObjectName name;
        ProcessHandle *process;

        bool supervisor;
        OsProcessStateFlags state;

        sm::FixedArray<std::byte> initArgs;
    };

    struct ProcessDestroyInfo {
        ProcessHandle *object;

        int64_t exitCode;
        OsProcessStateFlags reason;
    };

    struct ProcessStatInfo {
        ProcessHandle *object;
    };

    struct ProcessStat {
        ObjectName name;
        OsProcessId id;
        int64_t exitCode;
        OsProcessStateFlags state;
        sm::RcuSharedPtr<Process> parent;
        std::span<std::byte> args;
    };

    // thread

    struct ThreadCreateInfo {
        ObjectName name;

        RegisterSet cpuState;
        reg_t tlsAddress;
        OsThreadState state;
        OsSize kernelStackSize;
    };

    struct ThreadDestroyInfo {
        ThreadHandle *object;

        OsThreadState reason;
    };

    struct ThreadStat {
        ObjectName name;
        sm::RcuSharedPtr<Process> process;
        OsThreadState state;
    };

    // mutex

    struct MutexCreateInfo {
        ObjectName name;
        ProcessHandle *process;
    };

    struct MutexDestroyInfo {
        MutexHandle *object;
    };
}
