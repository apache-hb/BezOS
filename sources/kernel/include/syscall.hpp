#pragma once

#include <bezos/private.h>

#include "arch/msr.hpp"
#include "isr/isr.hpp"
#include "user/user.hpp"
#include "std/string.hpp"

#include <cstdint>

static constexpr x64::RwModelRegister<0xC0000080> IA32_EFER;

namespace km {
    class SystemMemory;

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

        uint64_t rflags;
        uint64_t arg3;
        uint64_t arg2;
        uint64_t arg1;
        uint64_t arg0;
        uint64_t function;
        uint64_t userStack;

        // system-v abi mandates the location of these
        uint64_t rbp;
        uint64_t rip;
    };

    static_assert(sizeof(SystemCallRegisterSet) == 112, "Update user.S to reflect changes in SystemCallRegisterSet");

    class CallContext {
        /// @brief Get the address space allocator for the calling process.
        PageTables& ptes();
    public:
        bool isMapped(uint64_t front, uint64_t back, PageFlags flags);

        /// @brief Read user memory.
        OsStatus readMemory(uint64_t address, size_t size, void *dst);

        /// @brief Write to user memory.
        OsStatus writeMemory(uint64_t address, const void *src, size_t size);

        /// @brief Read a single object from user memory.
        template<typename T>
        OsStatus readObject(uint64_t address, T *dst) {
            return readMemory(address, sizeof(T), dst);
        }

        /// @brief Read an array of memory into a container.
        template<typename T>
        OsStatus readArray(uint64_t front, uint64_t back, size_t limit, T *dst) {
            return km::CopyUserRange(ptes(), (void*)front, (void*)back, dst, limit);
        }

        template<typename T>
        OsStatus writeObject(uint64_t address, const T& src) {
            return writeMemory(address, &src, sizeof(T));
        }

        /// @brief Read a string from user memory.
        OsStatus readString(uint64_t front, uint64_t back, size_t limit, stdx::String *dst);

        /// @brief Read a range of memory into a pre-allocated buffer.
        OsStatus readRange(uint64_t front, uint64_t back, void *dst, size_t size);

        /// @brief Write a range of memory.
        OsStatus writeRange(uint64_t address, const void *front, const void *back);
    };

    using CallHandler = OsCallResult(*)(CallContext *ctx, SystemCallRegisterSet *regs);

    void SetupUserMode(SystemMemory *memory);

    void EnterUserMode(km::IsrContext state);

    void AddSystemCall(uint8_t function, CallHandler handler);

    template<typename T> requires (sizeof(T) <= sizeof(uint64_t))
    inline OsCallResult CallOk(T value) {
        return OsCallResult { .Status = OsStatusSuccess, .Value = std::bit_cast<uint64_t>(value) };
    }

    inline OsCallResult CallError(OsStatus status) {
        return OsCallResult { .Status = status, .Value = 0 };
    }
}
