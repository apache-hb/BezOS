#pragma once

#include <bezos/private.h>

#include "arch/msr.hpp"
#include "isr.hpp"

#include <cstdint>

namespace km {
    using SystemCallHandler = OsCallResult(*)(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3);

    static constexpr x64::ModelRegister<0xC0000080, x64::RegisterAccess::eReadWrite> kEfer;

    struct [[gnu::packed]] SystemCallContext {
        // user registers
        // r15 is clobbered by syscall
        uint64_t r14;
        uint64_t r13;
        uint64_t r12;
        // r11 is clobbered by syscall
        uint64_t r10;
        // r9 is clobbered by syscall
        // r8 is a parameter
        uint64_t rbx;
        uint64_t rbp;

        // syscall related
        uint64_t rip;
        uint64_t rflags;
        uint64_t arg3;
        uint64_t arg2;
        uint64_t arg1;
        uint64_t arg0;
        uint64_t function;
        uint64_t userStack;
    };

    void SetupUserMode();

    void EnterUserMode(km::IsrContext state);

    void AddSystemCall(uint8_t function, SystemCallHandler handler);

    template<typename T> requires (sizeof(T) <= sizeof(uint64_t))
    inline OsCallResult CallOk(T value) {
        return OsCallResult { .Status = OsStatusSuccess, .Value = std::bit_cast<uint64_t>(value) };
    }

    inline OsCallResult CallError(OsStatus status) {
        return OsCallResult { .Status = status, .Value = 0 };
    }
}
