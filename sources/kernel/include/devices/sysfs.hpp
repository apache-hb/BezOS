#pragma once

#include "fs/device.hpp"
#include "fs/folder.hpp"

#include "memory/range.hpp"

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
    namespace detail {
        void IdentifyAcpiTable(const acpi::RsdtHeader *header, OsIdentifyInfo *info);
        void IdentifyAcpiRoot(const acpi::AcpiTables *tables, OsIdentifyInfo *info);
        void IdentifySmbTable(const km::smbios::StructHeader *header, const km::SmBiosTables *tables, OsIdentifyInfo *info);
        void IdentifySmbRoot(const km::SmBiosTables *header, OsIdentifyInfo *info);

        OsStatus ReadTableData(km::VirtualRangeEx tables, vfs::ReadRequest request, vfs::ReadResult *result);
    }

    class AcpiTable;
    class AcpiRoot;
    class SmBiosTable;
    class SmBiosRoot;

    class AcpiTable final : public vfs::BasicNode {
        const acpi::RsdtHeader *mHeader;
    public:
        AcpiTable(const acpi::RsdtHeader *header);

        OsStatus query(sm::uuid uuid, const void *data, size_t size, vfs::IHandle **handle) override;
        OsStatus interfaces(OsIdentifyInterfaceList *list);
        OsStatus identify(OsIdentifyInfo *info);

        OsStatus stat(OsFileInfo *stat);
        OsStatus read(vfs::ReadRequest request, vfs::ReadResult *result);
    };

    class AcpiRoot final : public vfs::BasicNode, public vfs::FolderMixin {
        const acpi::AcpiTables *mTables;

    public:
        AcpiRoot(const acpi::AcpiTables *tables);

        OsStatus query(sm::uuid uuid, const void *data, size_t size, vfs::IHandle **handle) override;
        OsStatus interfaces(OsIdentifyInterfaceList *list);
        OsStatus identify(OsIdentifyInfo *info);

        OsStatus stat(OsFileInfo *stat);
        OsStatus read(vfs::ReadRequest request, vfs::ReadResult *result);

        static sm::RcuSharedPtr<AcpiRoot> create(sm::RcuDomain *domain, const acpi::AcpiTables *tables);
    };

    class SmBiosTable final : public vfs::BasicNode {
        const km::smbios::StructHeader *mHeader;
        const km::SmBiosTables *mTables;

    public:
        SmBiosTable(const km::smbios::StructHeader *header, const km::SmBiosTables *tables);

        OsStatus query(sm::uuid uuid, const void *data, size_t size, vfs::IHandle **handle) override;
        OsStatus interfaces(OsIdentifyInterfaceList *list);
        OsStatus identify(OsIdentifyInfo *info);

        OsStatus stat(OsFileInfo *stat);
        OsStatus read(vfs::ReadRequest request, vfs::ReadResult *result);
    };

    class SmBiosRoot final : public vfs::BasicNode, public vfs::FolderMixin {
        const km::SmBiosTables *mTables;

        vfs::NodeInfo mInfo;

    public:
        SmBiosRoot(const km::SmBiosTables *tables);

        OsStatus query(sm::uuid uuid, const void *data, size_t size, vfs::IHandle **handle) override;
        OsStatus interfaces(OsIdentifyInterfaceList *list);
        OsStatus identify(OsIdentifyInfo *info);

        OsStatus stat(OsFileInfo *stat);
        OsStatus read(vfs::ReadRequest request, vfs::ReadResult *result);

        static sm::RcuSharedPtr<SmBiosRoot> create(sm::RcuDomain *domain, const km::SmBiosTables *tables);
    };
}
