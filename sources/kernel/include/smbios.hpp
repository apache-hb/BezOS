#pragma once

#include "memory.hpp"
#include "std/static_string.hpp"

namespace km {
    struct PlatformInfo {
        static constexpr stdx::StringView kOracleVirtualBox = "innotek GmbH";

        using BiosString = stdx::StaticString<64>;

        /// @brief The vendor of the BIOS.
        BiosString vendor;

        /// @brief The version of the BIOS.
        BiosString version;

        BiosString manufacturer;
        BiosString product;
        BiosString serial;

        bool isOracleVirtualBox() const;
    };

    struct SmBiosLoadOptions {
        PhysicalAddress smbios32Address;
        PhysicalAddress smbios64Address;
        bool ignoreChecksum;
        bool ignore32BitEntry;
        bool ignore64BitEntry;
    };

    [[nodiscard]]
    OsStatus ReadSmbiosTables(SmBiosLoadOptions options, km::SystemMemory& memory, PlatformInfo *info [[gnu::nonnull]]);
}
