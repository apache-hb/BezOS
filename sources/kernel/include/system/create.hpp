#pragma once

#include "fs2/path.hpp"
#include "std/rcuptr.hpp"
#include "system/access.hpp" // IWYU pragma: export

#include "std/static_string.hpp"
#include "util/uuid.hpp"

namespace sys2 {
    using ObjectName = stdx::StaticString<OS_OBJECT_NAME_MAX>;

    using reg_t = uint64_t;

    struct RegisterSet {
        reg_t rax;
        reg_t rbx;
        reg_t rcx;
        reg_t rdx;
        reg_t rdi;
        reg_t rsi;
        reg_t r8;
        reg_t r9;
        reg_t r10;
        reg_t r11;
        reg_t r12;
        reg_t r13;
        reg_t r14;
        reg_t r15;
        reg_t rbp;
        reg_t rsp;
        reg_t rip;
        reg_t rflags;
    };

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

        vfs2::VfsPath path;
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
        vfs2::VfsPath path;
        sm::uuid interface;
        void *data;
        size_t size;
    };

    struct DeviceCloseInfo {

    };

    struct DeviceReadInfo {
        DeviceHandle *device;
        uint64_t offset;
        void *front;
        void *back;
        OsInstant timeout;
    };

    struct DeviceReadResult {
        uint64_t read;
    };

    struct DeviceWriteInfo {
        DeviceHandle *device;
        uint64_t offset;
        const void *front;
        const void *back;
        OsInstant timeout;
    };

    struct DeviceWriteResult {
        uint64_t write;
    };

    struct DeviceInvokeInfo {
        DeviceHandle *device;
        uint64_t method;
        void *data;
        size_t size;
    };

    struct DeviceStat {
        ObjectName name;
    };

    struct VmemCreateInfo {

    };

    struct VmemReleaseInfo {

    };

    struct VmemMapInfo {

    };

    // process

    struct ProcessCreateInfo {
        ObjectName name;
        ProcessHandle *process;

        bool supervisor;
        OsProcessStateFlags state;
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

    struct InvokeContext {
        /// @brief The system context.
        System *system;

        /// @brief The process namespace this method is being invoked in.
        ProcessHandle *process;

        /// @brief The thread in the process that is invoking the method.
        ThreadHandle *thread;

        /// @brief The transaction context that this method is contained in.
        TxHandle *tx;
    };
}
