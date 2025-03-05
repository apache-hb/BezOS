#pragma once

#include "fs2/node.hpp"

namespace km::smbios {
    struct StructHeader;
}

namespace acpi {
    struct RsdtHeader;
    class AcpiTables;
}

namespace km {
    struct SmBiosTables;
}

namespace dev {
    class AcpiTable : public vfs2::IVfsNode {
        const acpi::RsdtHeader *mHeader;

    public:
        AcpiTable(const acpi::RsdtHeader *header);
    };

    class AcpiRoot : public vfs2::IVfsNode {
        const acpi::AcpiTables *mTables;

    public:
        AcpiRoot(const acpi::AcpiTables *tables);
    };

    class SmBiosTable : public vfs2::IVfsNode {
        const km::smbios::StructHeader *mHeader;

    public:
        SmBiosTable(const km::smbios::StructHeader *header, const OsIdentifyInfo& info);
    };

    class SmBiosTables : public vfs2::IVfsNode {
        const km::SmBiosTables *mTables;

    public:
        SmBiosTables(const km::SmBiosTables *tables);
    };
}
