#pragma once

#include "arch/intrin.hpp"
#include "arch/paging.hpp"
#include "memory/layout.hpp"

namespace km {
    class PageManager {
        uintptr_t mAddressMask;
        uint64_t mHigherHalfOffset;

    public:
        constexpr PageManager(uintptr_t bits, uint64_t hhdmOffset) noexcept
            : mAddressMask(x64::paging::addressMask(bits))
            , mHigherHalfOffset(hhdmOffset)
        { }

        constexpr bool has3LevelPaging() const noexcept {
            return mAddressMask == x64::paging::addressMask(40);
        }

        constexpr bool has4LevelPaging() const noexcept {
            return mAddressMask == x64::paging::addressMask(48);
        }

        uintptr_t getAddressMask() const noexcept {
            return mAddressMask;
        }

        uint64_t hhdmOffset() const noexcept {
            return mHigherHalfOffset;
        }

        constexpr uintptr_t address(x64::Entry entry) const noexcept {
            return entry.underlying & mAddressMask;
        }

        constexpr void setAddress(x64::Entry& entry, uintptr_t address) const noexcept {
            entry.underlying = (entry.underlying & ~mAddressMask) | (address & mAddressMask);
        }

        km::PhysicalAddress activeMap() const noexcept {
            return (x64::cr3() & mAddressMask);
        }

        void setActiveMap(km::PhysicalAddress map) const noexcept {
            uint64_t reg = x64::cr3();
            reg = (reg & ~mAddressMask) | map.address;
            x64::setcr3(reg);
        }

        km::PhysicalAddress translate(km::VirtualAddress vaddr) const noexcept;
    };
}
