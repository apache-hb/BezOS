#pragma once

#include "arch/intrin.hpp"
#include "arch/paging.hpp"
#include "memory/layout.hpp"

namespace km {
    class PageManager {
        uintptr_t mAddressMask;
        uint64_t mHigherHalfOffset;

    public:
        constexpr PageManager(uintptr_t bits, uint64_t hhdmOffset)
            : mAddressMask(x64::paging::addressMask(bits))
            , mHigherHalfOffset(hhdmOffset)
        { }

        constexpr bool has3LevelPaging() const {
            return mAddressMask == x64::paging::addressMask(40);
        }

        constexpr bool has4LevelPaging() const {
            return mAddressMask == x64::paging::addressMask(48);
        }

        uintptr_t getAddressMask() const {
            return mAddressMask;
        }

        uint64_t hhdmOffset() const {
            return mHigherHalfOffset;
        }

        constexpr uintptr_t address(x64::Entry entry) const {
            return entry.underlying & mAddressMask;
        }

        constexpr void setAddress(x64::Entry& entry, uintptr_t address) const {
            entry.underlying = (entry.underlying & ~mAddressMask) | (address & mAddressMask);
        }

        km::PhysicalAddress activeMap() const {
            return (x64::cr3() & mAddressMask);
        }

        void setActiveMap(km::PhysicalAddress map) const {
            uint64_t reg = x64::cr3();
            reg = (reg & ~mAddressMask) | map.address;
            x64::setcr3(reg);
        }
    };
}
