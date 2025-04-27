#pragma once

#include <compare> // IWYU pragma: keep
#include <bit>

#include <cstddef>
#include <cstdint>

namespace sm {
    struct PhysicalAddress {
        uintptr_t address;

        constexpr PhysicalAddress() = default;

        constexpr PhysicalAddress(uintptr_t address)
            : address(address)
        { }

        constexpr PhysicalAddress(std::nullptr_t)
            : address(0)
        { }

        constexpr auto operator<=>(const PhysicalAddress&) const = default;

        constexpr PhysicalAddress& operator+=(intptr_t offset) noexcept {
            address += offset;
            return *this;
        }

        constexpr PhysicalAddress& operator-=(intptr_t offset) noexcept {
            address -= offset;
            return *this;
        }

        constexpr PhysicalAddress operator+(intptr_t offset) const noexcept {
            return PhysicalAddress { address + offset };
        }

        constexpr PhysicalAddress operator-(intptr_t offset) const noexcept {
            return PhysicalAddress { address - offset };
        }

        constexpr PhysicalAddress& operator--() noexcept {
            address -= 1;
            return *this;
        }

        constexpr PhysicalAddress operator--(int) noexcept {
            PhysicalAddress copy = *this;
            --(*this);
            return copy;
        }

        constexpr PhysicalAddress& operator++() noexcept {
            address += 1;
            return *this;
        }

        constexpr PhysicalAddress operator++(int) noexcept {
            PhysicalAddress copy = *this;
            ++(*this);
            return copy;
        }
    };
}
