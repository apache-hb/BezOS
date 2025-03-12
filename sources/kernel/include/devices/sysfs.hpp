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

        static constexpr inline auto kInterfaceList = std::to_array({
            kOsIdentifyGuid,
            kOsFileGuid,
        });

    public:
        AcpiTable(const acpi::RsdtHeader *header);

        OsStatus query(sm::uuid uuid, const void *data, size_t size, vfs2::IHandle **handle) override;
        OsStatus identify(OsIdentifyInfo *info);

        OsStatus stat(vfs2::NodeStat *stat);
        OsStatus read(vfs2::ReadRequest request, vfs2::ReadResult *result);

        std::span<const OsGuid> interfaces() { return kInterfaceList; }
    };

    class AcpiRoot final : public vfs2::BasicNode, public vfs2::FolderMixin {
        const acpi::AcpiTables *mTables;

        static constexpr inline auto kInterfaceList = std::to_array({
            kOsIdentifyGuid,
            kOsFolderGuid,
            kOsFileGuid,
        });

    public:
        AcpiRoot(const acpi::AcpiTables *tables);

        OsStatus query(sm::uuid uuid, const void *data, size_t size, vfs2::IHandle **handle) override;
        OsStatus identify(OsIdentifyInfo *info);

        OsStatus stat(vfs2::NodeStat *stat);
        OsStatus read(vfs2::ReadRequest request, vfs2::ReadResult *result);

        std::span<const OsGuid> interfaces() { return kInterfaceList; }
    };

    class SmBiosTable final : public vfs2::BasicNode {
        static constexpr inline auto kInterfaceList = std::to_array({
            kOsIdentifyGuid,
            kOsFileGuid,
        });

        const km::smbios::StructHeader *mHeader;
        const km::SmBiosTables *mTables;

    public:
        SmBiosTable(const km::smbios::StructHeader *header, const km::SmBiosTables *tables);

        OsStatus query(sm::uuid uuid, const void *data, size_t size, vfs2::IHandle **handle) override;
        OsStatus identify(OsIdentifyInfo *info);

        OsStatus stat(vfs2::NodeStat *stat);
        OsStatus read(vfs2::ReadRequest request, vfs2::ReadResult *result);

        std::span<const OsGuid> interfaces() { return kInterfaceList; }
    };

    class SmBiosRoot final : public vfs2::BasicNode, public vfs2::FolderMixin {
        const km::SmBiosTables *mTables;

        vfs2::NodeInfo mInfo;

        static constexpr inline auto kInterfaceList = std::to_array({
            kOsIdentifyGuid,
            kOsFolderGuid,
            kOsFileGuid,
        });

    public:
        SmBiosRoot(const km::SmBiosTables *tables);

        OsStatus query(sm::uuid uuid, const void *data, size_t size, vfs2::IHandle **handle) override;
        OsStatus identify(OsIdentifyInfo *info);

        OsStatus stat(vfs2::NodeStat *stat);
        OsStatus read(vfs2::ReadRequest request, vfs2::ReadResult *result);

        std::span<const OsGuid> interfaces() { return kInterfaceList; }
    };
}
