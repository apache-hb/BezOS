#pragma once

#include "util/digit.hpp"
#include <cstdint>
#include <utility>

namespace x64 {
    namespace idt {
        static constexpr uint8_t kFlagPresent = (1 << 7);
        static constexpr uint8_t kInterruptGate = 0b1110;
        // static constexpr uint8_t kTrapGate = 0b1111;
        // static constexpr uint8_t kTaskGate = 0b0101;
    }

    enum class Privilege : uint8_t {
        eSupervisor = 0,
        eUser = 3,
    };

    struct [[gnu::packed]] IdtEntry {
        uint16_t address0;
        uint16_t selector;
        uint8_t ist;
        uint8_t flags;
        sm::uint48_t address1;
        uint32_t reserved;

        constexpr bool isPresent() const noexcept {
            return (flags & idt::kFlagPresent) != 0;
        }

        constexpr Privilege privilege() const noexcept {
            return Privilege((flags >> 5) & 0b11);
        }

        constexpr bool isInterruptGate() const noexcept {
            return (flags & idt::kInterruptGate) == idt::kInterruptGate;
        }

        constexpr uintptr_t address() const noexcept {
            return (uint64_t(address1) << 16) | address0;
        }
    };

    static_assert(sizeof(IdtEntry) == 16);

    [[nodiscard]]
    constexpr x64::IdtEntry CreateIdtEntry(uintptr_t handler, uint16_t codeSelector, Privilege dpl, uint8_t ist) {
        uint8_t flags = x64::idt::kFlagPresent | x64::idt::kInterruptGate | ((std::to_underlying(dpl) & 0b11) << 5);

        return x64::IdtEntry {
            .address0 = uint16_t(handler & 0xFFFF),
            .selector = codeSelector,
            .ist = ist,
            .flags = flags,
            .address1 = sm::uint48_t((handler >> 16)),
        };
    }
}
