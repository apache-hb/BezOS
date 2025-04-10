#pragma once

#include "std/rcuptr.hpp"
#include "system/access.hpp" // IWYU pragma: export

#include "std/static_string.hpp"

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

    struct HandleCloneInfo {
        ProcessHandle *process;
        IHandle *handle;
        OsHandleAccess access;
    };

    struct HandleStat {
        OsHandleAccess access;
    };

    struct TxCreateInfo {
        ObjectName name;
        ProcessHandle *process;
        TxHandle *tx;
    };

    struct TxDestroyInfo {
        TxHandle *object;
        TxHandle *tx;
    };

    struct NodeOpenInfo {

    };

    struct NodeCloseInfo {

    };

    struct NodeQueryInfo {

    };

    struct NodeQueryResult {

    };

    struct NodeStatInfo {

    };

    struct NodeStatResult {

    };

    struct DeviceOpenInfo {

    };

    struct DeviceCloseInfo {

    };

    struct DeviceReadInfo {

    };

    struct DeviceReadResult {

    };

    struct DeviceWriteInfo {

    };

    struct DeviceWriteResult {

    };

    struct DeviceInvokeInfo {

    };

    struct DeviceInvokeResult {

    };

    struct DeviceStatInfo {

    };

    struct DeviceStatResult {

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
        TxHandle *tx;

        bool supervisor;
        OsProcessStateFlags state;
    };

    struct ProcessDestroyInfo {
        ProcessHandle *object;
        TxHandle *tx;

        int64_t exitCode;
        OsProcessStateFlags reason;
    };

    struct ProcessStatInfo {
        ProcessHandle *object;
        TxHandle *tx;
    };

    struct ProcessStatResult {
        ObjectName name;
        int64_t exitCode;
        OsProcessStateFlags state;
        sm::RcuSharedPtr<Process> parent;
    };

    // thread

    struct ThreadCreateInfo {
        ObjectName name;
        ProcessHandle *process;
        TxHandle *tx;

        RegisterSet cpuState;
        reg_t tlsAddress;
        OsThreadState state;
        OsSize kernelStackSize;
    };

    struct ThreadDestroyInfo {
        ThreadHandle *object;
        TxHandle *tx;

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
        TxHandle *tx;
    };

    struct MutexDestroyInfo {
        MutexHandle *object;
        TxHandle *tx;
    };

    struct InvokeContext {
        /// @brief The system context.
        System *system;

        /// @brief The process that is invoking the method.
        ProcessHandle *process;

        /// @brief The thread in the process that is invoking the method.
        ThreadHandle *thread;

        /// @brief The transaction context that this method is contained in.
        TxHandle *tx;
    };
}
