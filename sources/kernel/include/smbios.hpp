#pragma once

#include "memory.hpp"
#include "std/static_string.hpp"

namespace km {
    struct PlatformInfo {
        stdx::StaticString<64> vendor;
        stdx::StaticString<64> version;

        stdx::StaticString<64> manufacturer;
        stdx::StaticString<64> product;
        stdx::StaticString<64> serial;

        bool isOracleVirtualBox() const;
    };

    PlatformInfo GetPlatformInfo(
        km::PhysicalAddress smbios32Address,
        km::PhysicalAddress smbios64Address,
        km::SystemMemory& memory
    );
}
