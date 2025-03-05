#pragma once

#include "acpi/header.hpp"
#include "fs2/node.hpp"

namespace km::smbios {
    struct StructHeader;
}

namespace km {
    struct SmBiosTables;
}

namespace dev {
    class AcpiTable : public vfs2::IVfsNode {
        std::span<const uint8_t> mTable;

    public:
        AcpiTable(std::span<const uint8_t> table);
    };

    class SmBiosTable : public vfs2::IVfsNode {
        const km::smbios::StructHeader *mHeader;

    public:
        SmBiosTable(const km::smbios::StructHeader *header);
    };

    class SmBiosTables : public vfs2::IVfsNode {
        const km::SmBiosTables *mTables;

    public:
        SmBiosTables(const km::SmBiosTables *tables);
    };
}
