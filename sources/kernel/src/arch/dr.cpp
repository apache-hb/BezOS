#include "arch/debug.hpp"

using Dr7Format = km::Format<x64::DR7>;
using Dr6Format = km::Format<x64::DR6>;
using DebugStateFormat = km::Format<x64::DebugState>;

void Dr7Format::format(km::IOutStream& out, x64::DR7 value) {
    using x64::DR7;

    out.format("DR7 [ ", km::Hex(value.value()).pad(16));

    struct BitTest {
        DR7::Bit bit;
        stdx::StringView name;
    };
    static constexpr auto kBitTests = std::to_array<BitTest>({
        { DR7::L0,  "L0" },
        { DR7::G0,  "G0" },
        { DR7::L1,  "L1" },
        { DR7::G1,  "G1" },
        { DR7::L2,  "L2" },
        { DR7::G2,  "G2" },
        { DR7::L3,  "L3" },
        { DR7::G3,  "G3" },
        { DR7::LE,  "LE" },
        { DR7::GE,  "GE" },
        { DR7::RTM, "RTM" },
        { DR7::GD,  "GD" },
    });

    for (const auto& test : kBitTests) {
        if (value.test(test.bit)) {
            out.format(" ", test.name);
        }
    }

    out.format(" ]");
}

void Dr6Format::format(km::IOutStream& out, x64::DR6 value) {
    using x64::DR6;

    out.format("DR6 [ ", km::Hex(value.value()).pad(16));

    struct BitTest {
        DR6::Bit bit;
        stdx::StringView name;
    };
    static constexpr auto kBitTests = std::to_array<BitTest>({
        { DR6::B0, "B0" },
        { DR6::B1, "B1" },
        { DR6::B2, "B2" },
        { DR6::B3, "B3" },
        { DR6::BS, "BS" },
        { DR6::BD, "BD" },
        { DR6::RTM, "RTM" },
    });

    for (const auto& test : kBitTests) {
        if (value.test(test.bit)) {
            out.format(" ", test.name);
        }
    }

    out.format(" ]");
}

static stdx::StringView ModeName(x64::Mode mode) {
    switch (mode) {
    case x64::Mode::eDisabled:
        return "Disabled (0b00)";
    case x64::Mode::eGlobal:
        return "Global Breakpoint (0b10)";
    case x64::Mode::eLocal:
        return "Local Breakpoint (0b01)";
    default:
        return "Unknown";
    }
}

static stdx::StringView ConditionName(x64::Condition cond) {
    switch (cond) {
    case x64::Condition::X:
        return "Execute";
    case x64::Condition::W:
        return "Write";
    case x64::Condition::RW:
        return "Read/Write";
    case x64::Condition::RWX:
        return "Read/Write/Execute";
    default:
        return "Unknown";
    }
}

static stdx::StringView LengthName(x64::Length len) {
    switch (len) {
    case x64::Length::BYTE:
        return "byte";
    case x64::Length::WORD:
        return "word";
    case x64::Length::QWORD:
        return "qword";
    case x64::Length::DWORD:
        return "dword";
    default:
        return "Unknown";
    }
}

void DebugStateFormat::format(km::IOutStream& out, const x64::DebugState& value) {
    using x64::DR7, x64::DR6, x64::DebugState;

    auto dr7 = value.dr7();
    auto dr6 = value.dr6();
    auto addrs = value.addresses();

    auto fmtBreak = [&](unsigned reg) {
        auto config = dr7.getConfig(reg);
        auto cond = ConditionName(config.condition);
        auto size = LengthName(config.length);
        auto mode = ModeName(config.mode);

        out.format("BP", reg, ": ", mode, " ", cond, " breakpoint for ", size, " at ", km::Hex(addrs[reg]).pad(16), "\n");

        if (dr6.detected(reg)) {
            out.format("BP", reg, ": Triggered\n");
        }
    };

    out.format("=== DR Machine State ===\n");
    out.format("%DR7 | ", value.dr7(), "\n");
    out.format("%DR6 | ", value.dr6(), "\n");
    out.format("%DR3 | ", km::Hex(addrs[3]).pad(16), "\n");
    out.format("%DR2 | ", km::Hex(addrs[2]).pad(16), "\n");
    out.format("%DR1 | ", km::Hex(addrs[1]).pad(16), "\n");
    out.format("%DR0 | ", km::Hex(addrs[0]).pad(16), "\n");
    out.format("=== Debug Info ===\n");

    for (unsigned i = 0; i < x64::kBpCount; i++) {
        fmtBreak(i);
    }

    if (dr6.test(DR6::BS)) {
        out.format("Single step\n");
    }
}
