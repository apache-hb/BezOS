#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uintptr_t OsReg;

/// @brief Userspace visible thread context.
struct OsMachineContext {
    OsReg rax;
    OsReg rbx;
    OsReg rcx;
    OsReg rdx;
    OsReg rsi;
    OsReg rdi;
    OsReg rbp;
    OsReg rsp;
    OsReg rip;
    OsReg r8;
    OsReg r9;
    OsReg r10;
    OsReg r11;
    OsReg r12;
    OsReg r13;
    OsReg r14;
    OsReg r15;
};

#ifdef __cplusplus
}
#endif
