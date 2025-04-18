#pragma once

#include "arch/intrin.hpp"
#include "common/util/util.hpp"

#include "util/format.hpp"

#include <array>

namespace x64 {
    enum class Mode : uint64_t {
        eDisabled = 0b00,
        eLocal = 0b01,
        eGlobal = 0b10,

        eMask = 0b11,
    };

    enum class Condition : uint64_t {
        X = 0b00,
        W = 0b01,
        RWX = 0b10,
        RW = 0b11,

        eMask = 0b11,
    };

    enum class Length : uint64_t {
        BYTE = 0b00,
        WORD = 0b01,
        QWORD = 0b10,
        DWORD = 0b11,

        eMask = 0b11,
    };

    struct RegisterConfig {
        Condition condition;
        Length length;
        Mode mode;
    };

    struct Breakpoint {
        uintptr_t address;
        Condition condition;
        Length length;
        Mode mode;
    };

    constexpr inline auto kBpCount = 4u;

    class DR7 {
        uint64_t mValue;

        constexpr DR7(uint64_t value)
            : mValue(value)
        { }

    public:
        enum Bit : uint64_t {
            L0 = (1ull << 0),
            G0 = (1ull << 1),

            L1 = (1ull << 2),
            G1 = (1ull << 3),

            L2 = (1ull << 4),
            G2 = (1ull << 5),

            L3 = (1ull << 6),
            G3 = (1ull << 7),

            LE = (1ull << 8),
            GE = (1ull << 9),

            RTM = (1ull << 11),
            GD = (1ull << 13),
        };

        constexpr static DR7 of(uint64_t value) {
            return DR7(value | (1ull << 10));
        }

        constexpr uint64_t value() const noexcept { return mValue; }

        [[gnu::always_inline, gnu::nodebug]]
        static DR7 load() {
            return DR7(__get_dr7());
        }

        [[gnu::always_inline, gnu::nodebug]]
        static void store(DR7 value) {
            __set_dr7(value.mValue);
        }

        constexpr DR7& operator|=(Bit bit) {
            mValue |= std::to_underlying(bit);
            return *this;
        }

        constexpr DR7& operator&=(Bit bit) {
            mValue &= ~std::to_underlying(bit);
            return *this;
        }

        constexpr bool test(Bit bit) {
            return mValue & std::to_underlying(bit);
        }

        constexpr void setCondition(unsigned reg, Condition bp) {
            [[assume(reg <= 4)]];

            unsigned shift = (16 + (reg * 4));
            uint64_t mask = std::to_underlying(Condition::eMask) << shift;
            mValue = (mValue & ~mask) | (std::to_underlying(bp) << shift);
        }

        constexpr void setLength(unsigned reg, Length len) {
            [[assume(reg <= 4)]];

            unsigned shift = (18 + (reg * 4));
            uint64_t mask = std::to_underlying(Length::eMask) << shift;
            mValue = (mValue & ~mask) | (std::to_underlying(len) << shift);
        }

        constexpr void setMode(unsigned reg, Mode mode) {
            [[assume(reg <= 4)]];

            unsigned shift = (reg * 2);
            uint64_t mask = std::to_underlying(Mode::eMask) << shift;
            mValue = (mValue & ~mask) | (std::to_underlying(mode) << shift);
        }

        constexpr Condition getCondition(unsigned reg) {
            [[assume(reg <= 4)]];

            unsigned shift = (16 + (reg * 4));
            uint64_t mask = std::to_underlying(Condition::eMask) << shift;
            return static_cast<Condition>((mValue & mask) >> shift);
        }

        constexpr Length getLength(unsigned reg) {
            [[assume(reg <= 4)]];

            unsigned shift = (18 + (reg * 4));
            uint64_t mask = std::to_underlying(Length::eMask) << shift;
            return static_cast<Length>((mValue & mask) >> shift);
        }

        constexpr Mode getMode(unsigned reg) {
            [[assume(reg <= 4)]];

            unsigned shift = (reg * 2);
            uint64_t mask = std::to_underlying(Mode::eMask) << shift;
            return static_cast<Mode>((mValue & mask) >> shift);
        }

        constexpr void configure(unsigned reg, RegisterConfig bp) {
            setCondition(reg, bp.condition);
            setLength(reg, bp.length);
            setMode(reg, bp.mode);
        }

        constexpr RegisterConfig getConfig(unsigned reg) {
            return RegisterConfig {
                .condition = getCondition(reg),
                .length = getLength(reg),
                .mode = getMode(reg),
            };
        }
    };

    UTIL_BITFLAGS(DR7::Bit);

    class DR6 {
        uint64_t mValue;

        constexpr DR6(uint64_t value)
            : mValue(value)
        { }

    public:
        enum Bit : uint64_t {
            B0 = (1ull << 0),
            B1 = (1ull << 1),
            B2 = (1ull << 2),
            B3 = (1ull << 3),

            BLD = (1ull << 11),
            BD = (1ull << 13),
            BS = (1ull << 14),
            BT = (1ull << 15),
            RTM = (1ull << 16),
        };

        constexpr static DR6 of(uint64_t value) {
            return DR6(value | 0xFFFF'FFFF'0000'07F0);
        }

        constexpr uint64_t value() const noexcept { return mValue; }

        [[gnu::always_inline, gnu::nodebug]]
        static DR6 load() {
            return DR6(__get_dr6());
        }

        [[gnu::always_inline, gnu::nodebug]]
        static void store(DR6 value) {
            __set_dr6(value.mValue);
        }

        constexpr DR6& operator|=(Bit bit) {
            mValue |= std::to_underlying(bit);
            return *this;
        }

        constexpr DR6& operator&=(Bit bit) {
            mValue &= ~std::to_underlying(bit);
            return *this;
        }

        constexpr bool test(Bit bit) {
            return mValue & std::to_underlying(bit);
        }

        constexpr bool detected(unsigned reg) {
            [[assume(reg <= 4)]];

            return test(static_cast<Bit>(reg));
        }
    };

    UTIL_BITFLAGS(DR6::Bit);

    using DebugAddressSet = std::array<uintptr_t, 4>;

    class DebugState {
        DR7 mDebug7;
        DR6 mDebug6;
        DebugAddressSet mAddress;

        constexpr DebugState(DR7 dr7, DR6 dr6, DebugAddressSet address)
            : mDebug7(dr7), mDebug6(dr6), mAddress(address)
        { }

    public:
        static constexpr DebugState of(DR7 dr7, DR6 dr6, DebugAddressSet address) {
            return DebugState { dr7, dr6, address };
        }

        static DebugState load() {
            return DebugState {
                DR7::load(),
                DR6::load(),
                DebugAddressSet { __get_dr0(), __get_dr1(), __get_dr2(), __get_dr3() }
            };
        }

        static void store(DebugState state) {
            DR7::store(state.dr7());
            DR6::store(state.dr6());

            __set_dr0(state.drAddress(0));
            __set_dr1(state.drAddress(1));
            __set_dr2(state.drAddress(2));
            __set_dr3(state.drAddress(3));
        }

        constexpr DR7 dr7() const {
            return mDebug7;
        }

        constexpr DR6 dr6() const {
            return mDebug6;
        }

        constexpr DebugAddressSet addresses() const {
            return mAddress;
        }

        constexpr uintptr_t drAddress(unsigned reg) const {
            [[assume(reg <= 4)]];
            return mAddress[reg];
        }

        constexpr void configure(unsigned reg, Breakpoint bp) {
            [[assume(reg <= 4)]];

            mAddress[reg] = bp.address;
            mDebug7.configure(reg, RegisterConfig { bp.condition, bp.length, bp.mode });
        }

        constexpr Breakpoint getConfig(unsigned reg) {
            [[assume(reg <= 4)]];

            return Breakpoint {
                .address = mAddress[reg],
                .condition = mDebug7.getCondition(reg),
                .length = mDebug7.getLength(reg),
                .mode = mDebug7.getMode(reg),
            };
        }
    };
}

template<>
struct km::Format<x64::DR7> {
    static void format(km::IOutStream& out, x64::DR7 value);
};

template<>
struct km::Format<x64::DR6> {
    static void format(km::IOutStream& out, x64::DR6 value);
};

template<>
struct km::Format<x64::DebugState> {
    static void format(km::IOutStream& out, const x64::DebugState& value);
};
