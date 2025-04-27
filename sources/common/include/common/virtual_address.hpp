#pragma once

#include <compare> // IWYU pragma: keep - std::strong_ordering
#include <bit>
#include <memory> // IWYU pragma: keep - std::hash<>

#include <cstddef>
#include <cstdint>

namespace sm {
    struct VirtualAddress {
        uintptr_t address;

        constexpr VirtualAddress() noexcept = default;

        constexpr VirtualAddress(uintptr_t address) noexcept
            : address(address)
        { }

        constexpr VirtualAddress(std::nullptr_t) noexcept
            : address(0)
        { }

        VirtualAddress(const void *pointer) noexcept
            : address(std::bit_cast<uintptr_t>(pointer))
        { }

        operator const void*() const noexcept {
            return std::bit_cast<const void*>(address);
        }

        operator void*() noexcept {
            return std::bit_cast<void*>(address);
        }

        constexpr auto operator<=>(const VirtualAddress&) const noexcept = default;

        constexpr VirtualAddress& operator+=(intptr_t offset) noexcept {
            address += offset;
            return *this;
        }

        constexpr VirtualAddress& operator-=(intptr_t offset) noexcept {
            address -= offset;
            return *this;
        }

        constexpr VirtualAddress operator+(intptr_t offset) const noexcept {
            return VirtualAddress { address + offset };
        }

        constexpr VirtualAddress operator+(VirtualAddress offset) const noexcept {
            return VirtualAddress { address + offset.address };
        }

        constexpr VirtualAddress operator-(intptr_t offset) const noexcept {
            return VirtualAddress { address - offset };
        }

        constexpr VirtualAddress operator-(VirtualAddress offset) const noexcept {
            return VirtualAddress { address - offset.address };
        }

        constexpr uintptr_t operator%(uintptr_t offset) const noexcept {
            return address % offset;
        }

        constexpr VirtualAddress& operator--() noexcept {
            address -= 1;
            return *this;
        }

        constexpr VirtualAddress operator--(int) noexcept {
            VirtualAddress copy = *this;
            --(*this);
            return copy;
        }

        constexpr VirtualAddress& operator++() noexcept {
            address += 1;
            return *this;
        }

        constexpr VirtualAddress operator++(int) noexcept {
            VirtualAddress copy = *this;
            ++(*this);
            return copy;
        }
    };

    template<typename T>
    struct VirtualAddressOf : public VirtualAddress {
        using Type = T;

        constexpr VirtualAddressOf() noexcept = default;

        VirtualAddressOf(const T *pointer) noexcept
            : VirtualAddress(std::bit_cast<uintptr_t>(pointer))
        { }

        constexpr VirtualAddressOf& operator+=(intptr_t offset) noexcept {
            address += offset * sizeof(T);
            return *this;
        }

        constexpr VirtualAddressOf& operator-=(intptr_t offset) noexcept {
            address -= offset * sizeof(T);
            return *this;
        }

        constexpr VirtualAddressOf operator+(intptr_t offset) const noexcept {
            return VirtualAddressOf { address + offset * sizeof(T) };
        }

        constexpr VirtualAddressOf operator-(intptr_t offset) const noexcept {
            return VirtualAddressOf { address - offset * sizeof(T) };
        }

        constexpr VirtualAddressOf& operator++() noexcept {
            address += sizeof(T);
            return *this;
        }

        constexpr VirtualAddressOf operator++(int) noexcept {
            VirtualAddressOf copy = *this;
            ++(*this);
            return copy;
        }

        constexpr VirtualAddressOf& operator--() noexcept {
            address -= sizeof(T);
            return *this;
        }

        constexpr VirtualAddressOf operator--(int) noexcept {
            VirtualAddressOf copy = *this;
            --(*this);
            return copy;
        }

        T& operator[](intptr_t index) noexcept {
            return *std::bit_cast<T*>(address + index * sizeof(T));
        }

        const T& operator[](intptr_t index) const noexcept {
            return *std::bit_cast<const T*>(address + index * sizeof(T));
        }

        T& operator*() noexcept {
            return *std::bit_cast<T*>(address);
        }

        const T& operator*() const noexcept {
            return *std::bit_cast<const T*>(address);
        }

        T *operator->() noexcept {
            return std::bit_cast<T*>(address);
        }

        const T *operator->() const noexcept {
            return std::bit_cast<const T*>(address);
        }

        operator T*() noexcept {
            return std::bit_cast<T*>(address);
        }

        operator const T*() const noexcept {
            return std::bit_cast<const T*>(address);
        }
    };
}

template<>
struct std::hash<sm::VirtualAddress> {
    size_t operator()(const sm::VirtualAddress& address) const noexcept {
        return std::hash<uintptr_t>()(address.address);
    }
};

template<typename T>
struct std::hash<sm::VirtualAddressOf<T>> {
    size_t operator()(const sm::VirtualAddressOf<T>& address) const noexcept {
        return std::hash<uintptr_t>()(address.address);
    }
};
