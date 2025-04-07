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

        sm::RcuSharedPtr<Process> process;
        sm::RcuSharedPtr<Tx> tx;
    };

    struct ProcessCreateInfo {
        ObjectName name;

        sm::RcuSharedPtr<Process> parent;
        sm::RcuSharedPtr<Tx> tx;

        /// @brief Is this a supervisor process?
        bool supervisor;

        /// @brief Initial scheduling state.
        OsProcessStateFlags state;
    };

    struct ThreadCreateInfo {
        ObjectName name;

        sm::RcuSharedPtr<Process> process;
        sm::RcuSharedPtr<Tx> tx;

        /// @brief The initial cpu state for the thread.
        /// @note Only the syscall stack is allocated by the kernel, all other state is managed
        ///       by the userspace program. This puts the onus on the program to ensure that
        ///       exiting a thread will clean up all the state it has allocated aside from the thread
        ///       object itself, which is managed by the kernel.
        RegisterSet cpuState;
        reg_t tlsAddress;
        OsThreadState state;

        OsSize kernelStackSize;
    };

    struct MutexCreateInfo {
        ObjectName name;

        sm::RcuSharedPtr<Process> process;
        sm::RcuSharedPtr<Tx> tx;
    };
}
