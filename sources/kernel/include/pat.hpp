#pragma once

#include "common/util/util.hpp"

#include "memory/paging.hpp"

#include <stdint.h>

namespace x64 {
    namespace detail {
        void setPatEntry(uint64_t& pat, uint8_t index, km::MemoryType type);
        km::MemoryType getPatEntry(uint64_t pat, uint8_t index);
    }

    /// @brief Test if the processor supports the Page Attribute Table (PAT).
    ///
    /// @return True if the processor supports the PAT, false otherwise.
    bool hasPatSupport(void);

    /// @brief Test if the processor supports the Memory Type Range Registers (MTRRs).
    ///
    /// @return True if the processor supports the MTRRs, false otherwise.
    bool hasMtrrSupport(void);

    /// @brief Load the value of the PAT MSR.
    ///
    /// @pre HasPatSupport() = true
    ///
    /// @return The value of the PAT MSR.
    uint64_t loadPatMsr(void);

    class PageAttributeTable {
        uint64_t mValue;

        /// @brief Construct a page attribute table accessor.
        /// @pre: HasPatSupport() = true
        PageAttributeTable();
    public:

        constexpr PageAttributeTable(sm::noinit)
            : mValue(0)
        { }

        static PageAttributeTable get() { return PageAttributeTable(); }

        /// @brief Get the number of entries in the PAT.
        uint8_t count() const { return 8; }

        /// @brief Set the page type for an entry in the PAT.
        ///
        /// @pre @a index < count()
        ///
        /// @param index The index of the PAT entry.
        /// @param type The page type.
        void setEntry(uint8_t index, km::MemoryType type);

        /// @brief Get the page type for an entry in the PAT.
        ///
        /// @pre @a index < count()
        ///
        /// @param index The index of the PAT entry.
        /// @return The page type.
        km::MemoryType getEntry(uint8_t index) const;
    };

    class MemoryTypeRanges {
        uint64_t mMtrrCap;
        uint64_t mMtrrDefault;
        /// @brief Construct a memory type ranges accessor.
        /// @pre: HasMtrrSupport() = true
        MemoryTypeRanges();
    public:

        static MemoryTypeRanges get() { return MemoryTypeRanges(); }

        constexpr MemoryTypeRanges(sm::noinit)
            : mMtrrCap(0)
            , mMtrrDefault(0)
        { }

        class VariableMtrr {
            uint64_t mBase;
            uint64_t mMask;

        public:
            VariableMtrr(uint64_t base, uint64_t mask);

            km::MemoryType type() const;
            bool valid() const;

            km::PhysicalAddress baseAddress(const km::PageBuilder& pm) const;
            uintptr_t addressMask(const km::PageBuilder& pm) const;
        };

        VariableMtrr variableMtrr(uint8_t index) const;

        km::MemoryType fixedMtrr(uint8_t index) const;

        void setFixedMtrr(uint8_t index, km::MemoryType mtrr);

        void setVariableMtrr(uint8_t index, const km::PageBuilder& pm, km::MemoryType type, km::PhysicalAddress base, uintptr_t mask, bool enable);

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

        /// @brief Get the default MTRR memory type.
        /// @return The default memory type.
        km::MemoryType defaultType() const;

        void setDefaultType(km::MemoryType type);

        /// @brief Enable or disable MTRRs in the processor.
        /// @note This affects both fixed and variable MTRRs.
        void enable(bool enabled);
    };

    using VariableMtrr = MemoryTypeRanges::VariableMtrr;

    /// @brief Test if variable MTRRs are supported.
    /// @param mtrr The mtrr accessor.
    /// @return True if variable MTRRs are supported, false otherwise.
    inline bool HasVariableMtrrSupport(const MemoryTypeRanges& mtrr) {
        return mtrr.variableMtrrCount() > 0;
    }
}
