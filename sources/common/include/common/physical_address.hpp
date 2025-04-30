#pragma once

#include <compare> // IWYU pragma: keep - std::strong_ordering
#include <memory> // IWYU pragma: keep - std::hash<>

#include <cstddef>
#include <cstdint>

namespace sm {
    struct PhysicalAddress {
        uintptr_t address;

        constexpr PhysicalAddress() noexcept [[clang::nonblocking]] = default;

        constexpr PhysicalAddress(uintptr_t address) noexcept [[clang::nonblocking]]
            : address(address)
        { }

        constexpr PhysicalAddress(std::nullptr_t) noexcept [[clang::nonblocking]]
            : address(0)
        { }

        constexpr auto operator<=>(const PhysicalAddress&) const noexcept [[clang::nonblocking]] = default;

        constexpr PhysicalAddress& operator+=(ptrdiff_t offset) noexcept [[clang::nonblocking]] {
            address += offset;
            return *this;
        }

        constexpr PhysicalAddress& operator-=(ptrdiff_t offset) noexcept [[clang::nonblocking]] {
            address -= offset;
            return *this;
        }

        constexpr PhysicalAddress operator+(ptrdiff_t offset) const noexcept [[clang::nonblocking]] {
            return PhysicalAddress { address + offset };
        }

        constexpr PhysicalAddress operator+(PhysicalAddress offset) const noexcept [[clang::nonblocking]] {
            return PhysicalAddress { address + offset.address };
        }

        constexpr PhysicalAddress operator-(ptrdiff_t offset) const noexcept [[clang::nonblocking]] {
            return PhysicalAddress { address - offset };
        }

        constexpr ptrdiff_t operator-(PhysicalAddress offset) const noexcept [[clang::nonblocking]] {
            return  address - offset.address;
        }

        constexpr uintptr_t operator%(uintptr_t offset) const noexcept [[clang::nonblocking]] {
            return address % offset;
        }

        constexpr PhysicalAddress& operator--() noexcept [[clang::nonblocking]] {
            address -= 1;
            return *this;
        }

        constexpr PhysicalAddress operator--(int) noexcept [[clang::nonblocking]] {
            PhysicalAddress copy = *this;
            --(*this);
            return copy;
        }

        constexpr PhysicalAddress& operator++() noexcept [[clang::nonblocking]] {
            address += 1;
            return *this;
        }

        constexpr PhysicalAddress operator++(int) noexcept [[clang::nonblocking]] {
            PhysicalAddress copy = *this;
            ++(*this);
            return copy;
        }
    };
}

template<>
struct std::hash<sm::PhysicalAddress> {
    size_t operator()(const sm::PhysicalAddress& address) const noexcept {
        return std::hash<uintptr_t>()(address.address);
    }
};
