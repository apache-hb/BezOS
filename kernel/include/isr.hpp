#pragma once

#include "util/util.hpp"
#include "util/digit.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <climits>

namespace km {
    namespace isr {
        static constexpr uint8_t DE = 0x0;
        static constexpr uint8_t DB = 0x1;
        static constexpr uint8_t NMI = 0x2;
        static constexpr uint8_t BP = 0x3;
        static constexpr uint8_t OF = 0x4;
        static constexpr uint8_t BR = 0x5;
        static constexpr uint8_t UD = 0x6;
        static constexpr uint8_t NM = 0x7;
        static constexpr uint8_t DF = 0x8;
        static constexpr uint8_t TS = 0xA;
        static constexpr uint8_t NP = 0xB;
        static constexpr uint8_t SS = 0xC;
        static constexpr uint8_t GP = 0xD;
        static constexpr uint8_t PF = 0xE;
        static constexpr uint8_t MF = 0x10;
        static constexpr uint8_t AC = 0x11;
        static constexpr uint8_t MC = 0x12;
        static constexpr uint8_t XM = 0x13;
        static constexpr uint8_t VE = 0x14;
        static constexpr uint8_t CP = 0x15;

        /// @brief The number of exceptions reserved by the CPU
        static constexpr uint8_t kExceptionCount = 0x20;

        /// @brief The total number of ISRs the CPU supports
        static constexpr uint8_t kIsrCount = 0xFF;

        /// @brief The number of ISRs available for use
        static constexpr uint8_t kAvailableIsrCount = kIsrCount - kExceptionCount;
    }

    namespace irq {
        static constexpr uint8_t kTimer = 0x0;
        static constexpr uint8_t kKeyboard = 0x1;
        static constexpr uint8_t kCom2 = 0x3;
        static constexpr uint8_t kCom1 = 0x4;
        static constexpr uint8_t kLpt2 = 0x5;
        static constexpr uint8_t kFloppy = 0x6;
        static constexpr uint8_t kSpurious = 0x7;
        static constexpr uint8_t kClock = 0x8;
        static constexpr uint8_t kMouse = 0xC;
        static constexpr uint8_t kPrimaryAta = 0xE;
        static constexpr uint8_t kSecondaryAta = 0xF;
    }

    struct [[gnu::packed]] IdtEntry {
        uint16_t address0;
        uint16_t selector;
        uint8_t ist;
        uint8_t flags;
        sm::uint48_t address1;
        uint32_t reserved;
    };

    static_assert(sizeof(IdtEntry) == 16);


    struct [[gnu::packed]] IsrContext {
        uint64_t rax;
        uint64_t rbx;
        uint64_t rcx;
        uint64_t rdx;
        uint64_t rdi;
        uint64_t rsi;
        uint64_t r8;
        uint64_t r9;
        uint64_t r10;
        uint64_t r11;
        uint64_t r12;
        uint64_t r13;
        uint64_t r14;
        uint64_t r15;
        uint64_t rbp;

        uint64_t vector;
        uint64_t error;

        uint64_t rip;
        uint64_t cs;
        uint64_t rflags;
        uint64_t rsp;
        uint64_t ss;
    };

    static_assert(sizeof(IsrContext) == 176, "Update isr.S");

    using IsrCallback = km::IsrContext(*)(km::IsrContext*);

    km::IsrContext DefaultIsrHandler(km::IsrContext *context);

    /// @brief A table containing isr handlers for a single cpu.
    class [[gnu::packed]] IsrTable {
    public:
        static constexpr auto kCount = 256;
        using Entry = std::atomic<IsrCallback>;

    private:
        //
        // This syntax is to work around none of std::atomic<T>
        // being constexpr except the constructor. C++ has no way of expressing
        // an array with all its elements initialized to a single value so
        // we have to employ this gnu extension. But now this class is constexpr
        // constructible.
        //
        Entry mHandlers[kCount] = { [0 ... (kCount - 1)] = DefaultIsrHandler };

        Entry *find(const Entry *handle);

    public:
        IsrCallback install(uint8_t isr, IsrCallback callback);
        km::IsrContext invoke(km::IsrContext *context);

        const Entry *allocate(IsrCallback callback);
        void release(const Entry *callback);

        uint32_t index(const Entry *entry) const;
    };

    enum class Privilege : uint8_t {
        eSupervisor = 0,
        eUser = 3,
    };

    class IsrAllocator {
        static constexpr size_t kIsrCount = 256;
        uint8_t mFreeIsrs[kIsrCount / CHAR_BIT] = {};
    public:
        uint8_t claimIsr(uint8_t isr);
        void releaseIsr(uint8_t isr);

        uint8_t allocateIsr();
    };

    void DisableNmi();
    void EnableNmi();
    void DisableInterrupts();
    void EnableInterrupts();

    class IntGuard {
    public:
        UTIL_NOMOVE(IntGuard);
        UTIL_NOCOPY(IntGuard);

        IntGuard() {
            DisableInterrupts();
        }

        ~IntGuard() {
            EnableInterrupts();
        }
    };

    class NmiGuard : public IntGuard {
    public:
        UTIL_NOMOVE(NmiGuard);
        UTIL_NOCOPY(NmiGuard);

        NmiGuard() {
            DisableNmi();
        }

        ~NmiGuard() {
            EnableNmi();
        }
    };

    /// @brief Setup the global IDT.
    ///
    /// Called once during startup to populate the global IDT.
    ///
    /// @param isrs The isr allocator to populate.
    /// @param codeSelector the kernel code selector to use as the isr execution selector.
    void InitInterrupts(km::IsrAllocator& isrs, uint16_t codeSelector);

    /// @brief Setup the IDT for this core.
    ///
    /// Must be called before enabling interrupts or will lead to undefined behaviour.
    void LoadIdt();

    /// @brief Update an entry in the ist.
    ///
    /// Modify the global ist for a given entry to update its cs, dpl, and ist.
    ///
    /// @param isr The IDT entry to update.
    /// @param selector The new code selector for this entry.
    /// @param dpl The new privilige level for this entry.
    /// @param ist The new IST to use for this entry.
    void UpdateIdtEntry(uint8_t isr, uint16_t selector, Privilege dpl, uint8_t ist);

    IsrCallback InstallIsrHandler(uint8_t isr, IsrCallback handler);
}
