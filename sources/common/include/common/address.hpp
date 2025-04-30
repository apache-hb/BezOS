#pragma once

#include <compare> // IWYU pragma: keep - std::strong_ordering
#include <memory> // IWYU pragma: keep - std::hash<>

#include <cstddef>
#include <cstdint>

namespace sm {
    template<typename AddressSpace>
    struct Address {
        using Storage = typename AddressSpace::Storage;

        Storage address;

        constexpr Address() noexcept [[clang::nonblocking]] = default;

        constexpr Address(Storage address) noexcept [[clang::nonblocking]]
            : address(address)
        { }

        constexpr Address(std::nullptr_t) noexcept [[clang::nonblocking]]
            : address(0)
        { }

        constexpr auto operator<=>(const Address& other) const noexcept [[clang::nonblocking]] = default;

        constexpr bool isNull() const noexcept [[clang::nonblocking]] {
            return address == 0;
        }

        constexpr bool isAlignedTo(size_t alignment) const noexcept [[clang::nonblocking]] {
            return (address % alignment) == 0;
        }

        template<typename Self>
        constexpr std::remove_cvref_t<Self>& operator+=(this Self& self, ptrdiff_t offset) noexcept [[clang::nonblocking]] {
            self.address += offset;
            return self;
        }

        template<typename Self>
        constexpr std::remove_cvref_t<Self>& operator-=(this Self& self, ptrdiff_t offset) noexcept [[clang::nonblocking]] {
            self.address -= offset;
            return self;
        }

        template<typename Self>
        constexpr std::remove_cvref_t<Self> operator+(this Self self, ptrdiff_t offset) noexcept [[clang::nonblocking]] {
            return Self { self.address + offset };
        }

        template<typename Self>
        constexpr std::remove_cvref_t<Self> operator-(this Self self, ptrdiff_t offset) noexcept [[clang::nonblocking]] {
            return Self { self.address - offset };
        }

        constexpr ptrdiff_t operator-(Address offset) const noexcept [[clang::nonblocking]] {
            return address - offset.address;
        }

        constexpr uintptr_t operator%(uintptr_t offset) const noexcept [[clang::nonblocking]] {
            return address % offset;
        }

        template<typename Self>
        constexpr std::remove_cvref_t<Self>& operator--(this Self& self) noexcept [[clang::nonblocking]] {
            self.address -= 1;
            return self;
        }

        template<typename Self>
        constexpr std::remove_cvref_t<Self> operator--(this Self& self, int) noexcept [[clang::nonblocking]] {
            Self copy = self;
            --self;
            return copy;
        }

        template<typename Self>
        constexpr std::remove_cvref_t<Self>& operator++(this Self& self) noexcept [[clang::nonblocking]] {
            self += 1;
            return self;
        }

        template<typename Self>
        constexpr std::remove_cvref_t<Self> operator++(this Self& self, int) noexcept [[clang::nonblocking]] {
            Self copy = self;
            ++self;
            return copy;
        }
    };
}
