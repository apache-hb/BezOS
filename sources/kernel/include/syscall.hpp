#pragma once

#include <bezos/private.h>

#include "arch/msr.hpp"
#include "isr/isr.hpp"
#include "process/process.hpp"
#include "std/rcuptr.hpp"

#include <cstdint>

namespace km {
    class SystemMemory;

    using SystemCallHandler = OsCallResult(*)(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3);

    static constexpr x64::ModelRegister<0xC0000080, x64::RegisterAccess::eReadWrite> kEfer;

    struct [[gnu::packed]] SystemCallRegisterSet {
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

    template<typename T>
    class UserAddress {
        uintptr_t mValue;
    };

    template<typename T>
    class UserValue {
        uint64_t mValue;
    };

    class CallContext {
    public:
        sm::RcuSharedPtr<Process> process();
        sm::RcuSharedPtr<Thread> thread();
        PageTables& ptes();

        OsStatus readMemory(uint64_t address, size_t size, void *dst);
        OsStatus writeMemory(uint64_t address, const void *src, size_t size);

        template<typename T>
        OsStatus readObject(uint64_t address, T *dst);

        OsStatus readString(uint64_t front, uint64_t back, stdx::String *dst);

        OsStatus readRange(uint64_t front, uint64_t back, void *dst, size_t size);
        OsStatus writeRange(uint64_t address, const void *front, const void *back);
    };

    using BetterCallHandler = OsCallResult(*)(CallContext *ctx, SystemCallRegisterSet *regs);

    void SetupUserMode(SystemMemory *memory);

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
