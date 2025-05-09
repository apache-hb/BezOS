#pragma once

#include "arch/isr.hpp"
#include "common/util/util.hpp"

#include "util/digit.hpp"

#include "std/funqual.hpp"

#include <atomic>
#include <cstdint>

namespace km {
    /// @brief Standard architectural exception vector numbers.
    ///
    /// @details The ISR implementations are split across 4 files:
    /// - isr.S: The assembly ISR dispatch routine stubs.
    /// - isr.cpp: The IDT setup and management routines.
    /// - default.cpp: The default ISR handler.
    /// - tables.cpp: The ISR table implementations.
    ///
    /// The ISR implementations are split across these files to allow for
    /// testing of other components that require ISR functionality. ISRs
    /// are difficult to test in isolation due to the need for a full system
    /// to be running to invoke them. By seperating the implementation I can
    /// provide a userspace implementation with posix signals, allowing for
    /// comprehensive testing of the ISR table implementations in unit tests.
    namespace isr {
        // vectors for architectural exceptions

        constexpr uint8_t DE = 0x0;
        constexpr uint8_t DB = 0x1;
        constexpr uint8_t NMI = 0x2;
        constexpr uint8_t BP = 0x3;
        constexpr uint8_t OF = 0x4;
        constexpr uint8_t BR = 0x5;
        constexpr uint8_t UD = 0x6;
        constexpr uint8_t NM = 0x7;
        constexpr uint8_t DF = 0x8;
        constexpr uint8_t TS = 0xA;
        constexpr uint8_t NP = 0xB;
        constexpr uint8_t SS = 0xC;
        constexpr uint8_t GP = 0xD;
        constexpr uint8_t PF = 0xE;
        constexpr uint8_t MF = 0x10;
        constexpr uint8_t AC = 0x11;
        constexpr uint8_t MCE = 0x12;
        constexpr uint8_t XM = 0x13;
        constexpr uint8_t VE = 0x14;
        constexpr uint8_t CP = 0x15;

        /// @brief The number of exceptions reserved by the CPU
        constexpr auto kExceptionCount = 0x20;

        // vectors for common interrupts the OS uses

        /// @brief Local timer interrupt, used for per-cpu timer
        constexpr auto kTimerVector    = 0x20;

        /// @brief IPI communication vector
        constexpr auto kIpiVector      = 0x21;

        /// @brief Spurious interrupt, used for apic spurious interrupts
        constexpr auto kSpuriousVector = 0x22;

        constexpr auto kSharedCount = 0x23;

        /// @brief The total number of ISRs the CPU supports
        constexpr auto kIsrCount = 256;

        /// @brief The number of ISRs available to dynamically allocate
        constexpr auto kAvailableCount = kIsrCount - kSharedCount;
    }

    namespace irq {
        constexpr uint8_t kTimer = 0x0;
        constexpr uint8_t kKeyboard = 0x1;
        constexpr uint8_t kCom2 = 0x3;
        constexpr uint8_t kCom1 = 0x4;
        constexpr uint8_t kLpt2 = 0x5;
        constexpr uint8_t kFloppy = 0x6;
        constexpr uint8_t kSpurious = 0x7;
        constexpr uint8_t kClock = 0x8;
        constexpr uint8_t kMouse = 0xC;
        constexpr uint8_t kPrimaryAta = 0xE;
        constexpr uint8_t kSecondaryAta = 0xF;
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

    using IsrCallback = IsrContext(* [[clang::reentrant]])(IsrContext*);
    using IsrEntry = std::atomic<IsrCallback>;

    /// @brief The default ISR handler.
    ///
    /// This handler is invoked when an ISR is not installed.
    ///
    /// @param context The ISR context.
    ///
    /// @return The ISR context.
    IsrContext DefaultIsrHandler(IsrContext *context) [[clang::reentrant]];

    /// @brief A table of interrupt service routines.
    ///
    /// @tparam N The number of ISRs in the table.
    /// @tparam Offset The offset between the public ISR number and the actual ISR number.
    template<size_t N, size_t Offset>
    class IsrTableBase {
    public:
        /// @brief The number of ISRs in this table.
        static constexpr auto kCount = N;

        /// @brief The offset between the public ISR number and the actual ISR number.
        static constexpr auto kOffset = Offset;

    protected:
        //
        // This syntax is to work around none of std::atomic<T>
        // being constexpr except the constructor. C++ has no way of expressing
        // an array with all its elements initialized to a single value so
        // we have to employ this gnu extension. But now this class is constexpr
        // constructible.
        //
        IsrEntry mHandlers[N] = { [0 ... (N - 1)] = DefaultIsrHandler };

        uint8_t index(const IsrEntry *entry) const REENTRANT {
            return std::distance(mHandlers, entry) + kOffset;
        }

        IsrEntry *find(const IsrEntry *handle) REENTRANT {
            //
            // We need to be very certain what we're about do is alright.
            //
            if (std::begin(mHandlers) > handle || handle >= std::end(mHandlers)) {
                return nullptr;
            }

            //
            // This const_cast is sound as we are certain it points to a member of the
            // array we own. And we are being executed in a non-const context, so this
            // is exactly equal to doing the work required to constitute the array element.
            //
            return const_cast<IsrEntry*>(handle);
        }

        const IsrEntry *allocate(IsrCallback callback) REENTRANT {
            for (IsrEntry& entry : mHandlers) {

                //
                // Find the first entry that is free by swapping with the default handler.
                // All free entries are denoted by containing `DefaultIsrHandler`.
                //
                IsrCallback expected = DefaultIsrHandler;
                if (entry.compare_exchange_strong(expected, callback)) {
                    return &entry;
                }
            }

            return nullptr;
        }

        bool release(const IsrEntry *callback, IsrCallback expected) REENTRANT {
            IsrEntry *entry = find(callback);

            //
            // Only swap out the ISR handler with the default one if
            // it is still set to what the caller provided us. If it
            // has changed since then another thread has beaten us to
            // reusing it and we don't want to trample their work.
            //
            return entry->compare_exchange_strong(expected, DefaultIsrHandler);
        }

    public:
        constexpr IsrTableBase() = default;

        IsrCallback install(uint8_t isr, IsrCallback callback) REENTRANT {
            return mHandlers[isr - kOffset].exchange(callback);
        }

        IsrContext invoke(IsrContext *context) SIGNAL_HANDLER {
            QTAG_INDIRECT(signal_handler) IsrCallback isr = mHandlers[uint8_t(context->vector) - kOffset];
            return isr(context);
        }

        IsrEntry *begin() REENTRANT { return mHandlers; }
        IsrEntry *end() REENTRANT { return mHandlers + N; }
    };

    /// @brief A table containing x86 exception handlers for all cpus.
    ///
    /// The first 32 entries are reserved by the CPU for exceptions.
    /// We share this table across all cpus as the handlers are the same.
    ///
    /// This table is preserved for the lifetime of the system, and is
    /// never deallocated.
    class SharedIsrTable : public IsrTableBase<isr::kSharedCount, 0> {
    public:
        using IsrTableBase::install;
        using IsrTableBase::invoke;
    };

    /// @brief A table containing isr handlers for a single cpu.
    ///
    /// The remaining entries in the x86 idt are available for use as
    /// interrupt service routines. Each cpu has its own table to allow
    /// for per-cpu interrupt handling.
    ///
    /// This table is swapped out during the initial boot process and
    /// replaced with a cpu local table. If you install an isr during
    /// the initial boot sequence and want to keep it, you must reinstall
    /// it on the cpu local table.
    class LocalIsrTable : public IsrTableBase<isr::kAvailableCount, isr::kSharedCount> {
    public:
        using IsrTableBase::install;
        using IsrTableBase::invoke;
        using IsrTableBase::index;

        using IsrTableBase::find;
        using IsrTableBase::allocate;
        using IsrTableBase::release;
    };

    class ILocalIsrManager {
    public:
        virtual ~ILocalIsrManager() = default;

        virtual LocalIsrTable *getLocalIsrTable() = 0;
    };

    void DisableNmi();
    void EnableNmi();
    void DisableInterrupts();
    void EnableInterrupts();

    /// @brief RAII guard to disable interrupts.
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

    /// @brief RAII guard to disable NMI.
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

    SharedIsrTable *GetSharedIsrTable() REENTRANT;
    LocalIsrTable *GetLocalIsrTable();
    void SetIsrManager(ILocalIsrManager *manager);

    void EnableCpuLocalIsrTable();
    void InitCpuLocalIsrTable();

    /// @brief Setup the global IDT.
    ///
    /// Called once during startup to populate the global IDT.
    ///
    /// @param cs the kernel code selector to use as the isr execution selector.
    void InitInterrupts(uint16_t cs);

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
    void UpdateIdtEntry(uint8_t isr, uint16_t selector, x64::Privilege dpl, uint8_t ist);
}
