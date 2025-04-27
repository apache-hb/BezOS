#pragma once

#include <compare> // IWYU pragma: keep
#include <string_view>

#include <stdint.h>

namespace sm {
    class Version {
        uint32_t mVersion;

    public:
        constexpr Version(uint8_t major, uint8_t minor, uint16_t patch) noexcept
            : mVersion((static_cast<uint32_t>(major) << 16) | (static_cast<uint32_t>(minor) << 8) | static_cast<uint32_t>(patch))
        { }

        constexpr uint8_t major() const noexcept {
            return static_cast<uint8_t>(mVersion >> 16);
        }

        constexpr uint8_t minor() const noexcept {
            return static_cast<uint8_t>((mVersion >> 8) & 0xFF);
        }

        constexpr uint16_t patch() const noexcept {
            return static_cast<uint16_t>(mVersion & 0xFFFF);
        }

        constexpr auto operator<=>(const Version& other) const noexcept = default;
    };

    struct GenericCompiler {
        [[gnu::error("CompilerName not implemented")]]
        static constexpr std::string_view GetName() noexcept;

        [[gnu::error("CompilerVersion not implemented")]]
        static constexpr Version GetVersion() noexcept;
    };
}
