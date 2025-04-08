#pragma once

#include "std/rcuptr.hpp"
#include "system/access.hpp" // IWYU pragma: export

#include "std/static_string.hpp"
#include "util/absl.hpp"

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

    struct TxCreateInfo {
        ObjectName name;
        ProcessHandle *process;
        TxHandle *tx;
    };

    struct ProcessCreateInfo {
        ObjectName name;
        ProcessHandle *process;
        TxHandle *tx;

        bool supervisor;
        OsProcessStateFlags state;
    };

    struct ThreadCreateInfo {
        ObjectName name;
        ProcessHandle *process;
        TxHandle *tx;

        RegisterSet cpuState;
        reg_t tlsAddress;
        OsThreadState state;
        OsSize kernelStackSize;
    };

    struct MutexCreateInfo {
        ObjectName name;
        ProcessHandle *process;
        TxHandle *tx;
    };

    struct ProcessDestroyInfo {
        ProcessHandle *process;
        TxHandle *tx;

        int64_t exitCode;
        OsProcessStateFlags reason;
    };

    struct ThreadDestroyInfo {
        ThreadHandle *thread;
        TxHandle *tx;

        OsThreadState reason;
    };

    struct InvokeContext {
        /// @brief The system context.
        System *system;

        /// @brief The process that is invoking the method.
        ProcessHandle *process;

        /// @brief The thread in the process that is invoking the method.
        ThreadHandle *thread;
    };
}
