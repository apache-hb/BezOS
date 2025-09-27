#pragma once

#include "common/address.hpp"

#include <compare> // IWYU pragma: keep - std::strong_ordering
#include <memory> // IWYU pragma: keep - std::hash<>

#include <cstddef>
#include <cstdint>

namespace sm {
    namespace detail {
        struct PhysicalAddressSpace {
            using Storage = uintptr_t;
        };
    }

    struct PhysicalAddress : public sm::Address<detail::PhysicalAddressSpace> {
        using Address::Address;

        static constexpr PhysicalAddress invalid() noexcept {
            return PhysicalAddress { UINTPTR_MAX };
        }
    };
}

template<>
struct std::hash<sm::PhysicalAddress> {
    size_t operator()(const sm::PhysicalAddress& address) const noexcept {
        return std::hash<uintptr_t>()(address.address);
    }
};
