#pragma once

#include "fs2/device.hpp"
#include "fs2/folder.hpp"

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

        OsStatus ReadTableData(km::VirtualRange tables, vfs2::ReadRequest request, vfs2::ReadResult *result);
    }

    class AcpiTable;
    class AcpiRoot;
    class SmBiosTable;
    class SmBiosRoot;

    class AcpiTable final : public vfs2::BasicNode {
        const acpi::RsdtHeader *mHeader;
    public:
        AcpiTable(const acpi::RsdtHeader *header);

        OsStatus query(sm::uuid uuid, const void *data, size_t size, vfs2::IHandle **handle) override;
        OsStatus interfaces(OsIdentifyInterfaceList *list);
        OsStatus identify(OsIdentifyInfo *info);

        OsStatus stat(OsFileInfo *stat);
        OsStatus read(vfs2::ReadRequest request, vfs2::ReadResult *result);
    };

    class AcpiRoot final : public vfs2::BasicNode, public vfs2::FolderMixin {
        const acpi::AcpiTables *mTables;

    public:
        AcpiRoot(const acpi::AcpiTables *tables);

        OsStatus query(sm::uuid uuid, const void *data, size_t size, vfs2::IHandle **handle) override;
        OsStatus interfaces(OsIdentifyInterfaceList *list);
        OsStatus identify(OsIdentifyInfo *info);

        OsStatus stat(OsFileInfo *stat);
        OsStatus read(vfs2::ReadRequest request, vfs2::ReadResult *result);

        static sm::RcuSharedPtr<AcpiRoot> create(sm::RcuDomain *domain, const acpi::AcpiTables *tables);
    };

    class SmBiosTable final : public vfs2::BasicNode {
        const km::smbios::StructHeader *mHeader;
        const km::SmBiosTables *mTables;

    public:
        SmBiosTable(const km::smbios::StructHeader *header, const km::SmBiosTables *tables);

        OsStatus query(sm::uuid uuid, const void *data, size_t size, vfs2::IHandle **handle) override;
        OsStatus interfaces(OsIdentifyInterfaceList *list);
        OsStatus identify(OsIdentifyInfo *info);

        OsStatus stat(OsFileInfo *stat);
        OsStatus read(vfs2::ReadRequest request, vfs2::ReadResult *result);
    };

    class SmBiosRoot final : public vfs2::BasicNode, public vfs2::FolderMixin {
        const km::SmBiosTables *mTables;

        vfs2::NodeInfo mInfo;

    public:
        SmBiosRoot(const km::SmBiosTables *tables);

        OsStatus query(sm::uuid uuid, const void *data, size_t size, vfs2::IHandle **handle) override;
        OsStatus interfaces(OsIdentifyInterfaceList *list);
        OsStatus identify(OsIdentifyInfo *info);

        OsStatus stat(OsFileInfo *stat);
        OsStatus read(vfs2::ReadRequest request, vfs2::ReadResult *result);

        static sm::RcuSharedPtr<SmBiosRoot> create(sm::RcuDomain *domain, const km::SmBiosTables *tables);
    };
}
