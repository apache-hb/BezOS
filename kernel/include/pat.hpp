#pragma once

#include "util/util.hpp"

#include <stdint.h>

namespace x64 {
    /// @brief Page Attribute Table (PAT) page types.
    /// @see AMD64 Architecture Programmer's Manual Volume 2: System Programming Table 7-9.
    enum class PageType {
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

    bool HasPatSupport(void);
    bool HasMtrrSupport(void);

    class PageAttributeTable {
        uint64_t mValue;

    public:
        /// @pre: HasPatSupport() = true
        PageAttributeTable();

        PageAttributeTable(sm::noinit)
            : mValue(0)
        { }

        void setEntry(uint8_t index, PageType type);
    };

    class MemoryTypeRanges {
        uint64_t mMtrrCap;
        uint64_t mMtrrDefault;
    public:
        /// @pre: HasMtrrSupport() = true
        MemoryTypeRanges();

        MemoryTypeRanges(sm::noinit)
            : mMtrrCap(0)
            , mMtrrDefault(0)
        { }

        uint8_t mtrrCount() const;

        bool fixedRangeMtrrEnabled() const;
        bool hasAllFixedRangeMtrrs() const;

        bool hasWriteCombining() const;

        bool enabled() const;
    };
}
