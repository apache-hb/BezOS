#pragma once

#include "memory/layout.hpp"
#include "memory/paging.hpp"
#include "util/util.hpp"
#include "util/format.hpp"

#include <stdint.h>

namespace x64 {
    /// @brief Page Attribute Table (PAT) and Memory Type Range Registers (MTRRs) choices.
    /// @see AMD64 Architecture Programmer's Manual Volume 2: System Programming Table 7-9.
    enum class MemoryType {
        /// @brief No caching, write combining, or speculative reads are allowed.
        eUncached            = 0x00,

        /// @brief Only write combining is allowed,
        eWriteCombined       = 0x01,

        /// @brief Only speculative reads are allowed.
        eWriteThrough        = 0x04,

        /// @brief All writes update main memory.
        eWriteProtect        = 0x05,

        /// @brief All reads and writes can be cached.
        eWriteBack           = 0x06,

        /// @brief Same as @a eUncached, but can be overridden by the MTRRs.
        /// @note Not valid for @a MemoryTypeRanges.
        eUncachedOverridable = 0x07,
    };

    /// @brief Test if the processor supports the Page Attribute Table (PAT).
    ///
    /// @return True if the processor supports the PAT, false otherwise.
    bool HasPatSupport(void);

    /// @brief Test if the processor supports the Memory Type Range Registers (MTRRs).
    ///
    /// @return True if the processor supports the MTRRs, false otherwise.
    bool HasMtrrSupport(void);

    class PageAttributeTable {
        uint64_t mValue;

    public:
        /// @brief Construct a page attribute table accessor.
        /// @pre: HasPatSupport() = true
        PageAttributeTable();

        constexpr PageAttributeTable(sm::noinit)
            : mValue(0)
        { }

        /// @brief Get the number of entries in the PAT.
        uint8_t count() const { return 8; }

        /// @brief Set the page type for an entry in the PAT.
        ///
        /// @pre @a index < count()
        ///
        /// @param index The index of the PAT entry.
        /// @param type The page type.
        void setEntry(uint8_t index, MemoryType type);

        /// @brief Get the page type for an entry in the PAT.
        ///
        /// @pre @a index < count()
        ///
        /// @param index The index of the PAT entry.
        /// @return The page type.
        MemoryType getEntry(uint8_t index) const;
    };

    class MemoryTypeRanges {
        uint64_t mMtrrCap;
        uint64_t mMtrrDefault;
    public:
        /// @brief Construct a memory type ranges accessor.
        /// @pre: HasMtrrSupport() = true
        MemoryTypeRanges();

        constexpr MemoryTypeRanges(sm::noinit)
            : mMtrrCap(0)
            , mMtrrDefault(0)
        { }

        class VariableMtrr {
            uint64_t mBase;
            uint64_t mMask;

        public:
            VariableMtrr(uint64_t base, uint64_t mask);

            MemoryType type() const;
            bool valid() const;

            km::PhysicalAddress baseAddress(const km::PageManager& pm) const;
            uintptr_t addressMask(const km::PageManager& pm) const;
        };

        class FixedMtrr {
            uint8_t mValue;

        public:
            FixedMtrr(uint8_t value);

            MemoryType type() const;
        };

        VariableMtrr variableMtrr(uint8_t index) const;

        FixedMtrr fixedMtrr(uint8_t index) const;

        /// @brief Get the number of variable MTRRs supported by the processor.
        /// @return The number of variable MTRRs.
        uint8_t variableMtrrCount() const;

        /// @brief Get the number of fixed range MTRRs supported by the processor.
        /// @return The number of fixed range MTRRs.
        uint8_t fixedMtrrCount() const { return 11 * 8; }

        /// @brief Test if the processor supports the fixed range MTRRs.
        /// @return True if the processor supports the fixed range MTRRs, false otherwise.
        bool fixedMtrrSupported() const;

        /// @brief Test if the fixed range MTRRs are enabled in the processor.
        /// @return True if the fixed range MTRRs are enabled, false otherwise.
        bool fixedMtrrEnabled() const;

        /// @brief Enable or disable the fixed range MTRRs in the processor.
        /// @param enabled True to enable the fixed range MTRRs, false to disable them.
        void enableFixedMtrrs(bool enabled);

        /// @brief Test if the processor supports write combining MTRRs.
        /// @return true if the processor supports write combining MTRRs, false otherwise.
        bool hasWriteCombining() const;

        /// @brief Test if MTRRs are enabled in the processor.
        /// @return true if MTRRs are enabled, false otherwise.
        bool enabled() const;

        /// @brief Enable or disable MTRRs in the processor.
        /// @note This affects both fixed and variable MTRRs.
        void enable(bool enabled);
    };

    using FixedMtrr = MemoryTypeRanges::FixedMtrr;
    using VariableMtrr = MemoryTypeRanges::VariableMtrr;

    /// @brief Test if variable MTRRs are supported.
    /// @param mtrr The mtrr accessor.
    /// @return True if variable MTRRs are supported, false otherwise.
    inline bool HasVariableMtrrSupport(const MemoryTypeRanges& mtrr) {
        return mtrr.variableMtrrCount() > 0;
    }
}

template<>
struct km::StaticFormat<x64::MemoryType> {
    static constexpr size_t kStringSize = 16;
    static constexpr stdx::StringView toString(char*, x64::MemoryType type) {
        using namespace stdx::literals;
        using enum x64::MemoryType;

        switch (type) {
        case eUncached:
            return "Uncached"_sv;
        case eWriteCombined:
            return "Write Combined"_sv;
        case eWriteThrough:
            return "Write Through"_sv;
        case eWriteProtect:
            return "Write Protect"_sv;
        case eWriteBack:
            return "Write Back"_sv;
        case eUncachedOverridable:
            return "Uncached-"_sv;
        }
    }
};
