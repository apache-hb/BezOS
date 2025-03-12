#include "devices/sysfs.hpp"

#include "acpi/acpi.hpp"
#include "acpi/header.hpp"
#include "fs2/file.hpp"
#include "fs2/identify.hpp"
#include "smbios.hpp"

namespace smbios = km::smbios;

namespace stdr = std::ranges;

void dev::detail::IdentifyAcpiTable(const acpi::RsdtHeader *header, OsIdentifyInfo *info) {
    OsIdentifyInfo result = {
        .DriverVendor = "BezOS",
        .DriverVersion = OS_VERSION(1, 0, 0),
    };

    stdr::copy(header->signature, result.DisplayName);
    stdr::copy(header->oemid, result.DeviceVendor);
    stdr::copy(header->tableId, result.Model);

    if (header->revision == 0) {
        stdr::copy("1.0", result.FirmwareRevision);
    } else {
        stdr::copy("2.0", result.FirmwareRevision);
    }

    *info = result;
}

void dev::detail::IdentifyAcpiRoot(const acpi::AcpiTables *tables, OsIdentifyInfo *info) {
    const auto *rsdp = tables->locator();

    OsIdentifyInfo result {
        .DisplayName = "ACPI",
        .DriverVendor = "BezOS",
        .DriverVersion = OS_VERSION(1, 0, 0),
    };

    stdr::copy(rsdp->oemid, result.DeviceVendor);

    if (rsdp->revision == 0) {
        stdr::copy("1.0", result.FirmwareRevision);
    } else {
        stdr::copy("2.0", result.FirmwareRevision);
    }

    *info = result;
}

void dev::detail::IdentifySmbTable(const km::smbios::StructHeader *header, const km::SmBiosTables *tables, OsIdentifyInfo *info) {
    detail::IdentifySmbRoot(tables, info);

    auto tableName = km::smbios::TableName(header->type);
    stdr::copy(tableName, info->DisplayName);

    auto handle = km::format(km::Hex(header->handle).pad(4));
    stdr::copy(handle, info->Serial);
}

OsStatus dev::detail::ReadTableData(km::VirtualRange tables, vfs2::ReadRequest request, vfs2::ReadResult *result) {
    uint64_t front = request.offset;
    uint64_t back = request.offset + request.size();

    if (front >= tables.size()) {
        return OsStatusEndOfFile;
    }

    if (back > tables.size()) {
        back = tables.size();
    }

    size_t count = back - front;
    memcpy(request.begin, (char*)tables.front + front, count);
    result->read = count;
    return OsStatusSuccess;
}

void dev::detail::IdentifySmbRoot(const km::SmBiosTables *header, OsIdentifyInfo *info) {
    const auto *firmware = header->firmwareInfo();
    const auto *system = header->systemInfo();

    stdx::StringView vendor = smbios::GetStringEntry(firmware, firmware->vendor);
    stdx::StringView version = smbios::GetStringEntry(firmware, firmware->version);
    stdx::StringView manufacturer = smbios::GetStringEntry(system, system->manufacturer);
    stdx::StringView product = smbios::GetStringEntry(system, system->productName);
    stdx::StringView serial = smbios::GetStringEntry(system, system->serialNumber);

    OsIdentifyInfo result {
        .DriverVendor = "BezOS",
        .DriverVersion = OS_VERSION(1, 0, 0),
    };

    stdr::copy(vendor, result.DisplayName);
    stdr::copy(product, result.Model);
    stdr::copy(serial, result.Serial);
    stdr::copy(manufacturer, result.DeviceVendor);
    stdr::copy(version, result.FirmwareRevision);

    *info = result;
}

dev::AcpiTable::AcpiTable(const acpi::RsdtHeader *table)
    : mHeader(table)
{ }

OsStatus dev::AcpiTable::query(sm::uuid uuid, const void *, size_t, vfs2::IHandle **handle) {
    if (uuid == kOsIdentifyGuid) {
        auto *identify = new vfs2::TIdentifyHandle<AcpiTable>(this);
        if (!identify) {
            return OsStatusOutOfMemory;
        }

        *handle = identify;
        return OsStatusSuccess;
    }

    if (uuid == kOsFileGuid) {
        auto *file = new vfs2::TFileHandle<AcpiTable>(this);
        if (!file) {
            return OsStatusOutOfMemory;
        }

        *handle = file;
        return OsStatusSuccess;
    }

    return OsStatusInterfaceNotSupported;
}

OsStatus dev::AcpiTable::identify(OsIdentifyInfo *info) {
    detail::IdentifyAcpiTable(mHeader, info);
    return OsStatusSuccess;
}

OsStatus dev::AcpiTable::stat(vfs2::NodeStat *stat) {
    km::VirtualRange range = km::VirtualRange::of((void*)mHeader, mHeader->length);

    *stat = vfs2::NodeStat {
        .logical = range.size(),
        .blksize = 1,
        .blocks = range.size(),
        .access = vfs2::Access::R,
    };

    return OsStatusSuccess;
}

OsStatus dev::AcpiTable::read(vfs2::ReadRequest request, vfs2::ReadResult *result) {
    km::VirtualRange range = km::VirtualRange::of((void*)mHeader, mHeader->length);
    return detail::ReadTableData(range, request, result);
}

//
// acpi locator
//

dev::AcpiRoot::AcpiRoot(const acpi::AcpiTables *tables)
    : mTables(tables)
{
    for (const acpi::RsdtHeader *header : mTables->entries()) {
        dev::AcpiTable *table = new dev::AcpiTable(header);
        mknode(vfs2::VfsStringView(header->signature), table);
    }
}

OsStatus dev::AcpiRoot::query(sm::uuid uuid, const void *, size_t, vfs2::IHandle **handle) {
    if (uuid == kOsIdentifyGuid) {
        auto *identify = new vfs2::TIdentifyHandle<AcpiRoot>(this);
        if (!identify) {
            return OsStatusOutOfMemory;
        }

        *handle = identify;
        return OsStatusSuccess;
    }

    if (uuid == kOsFileGuid) {
        auto *file = new vfs2::TFileHandle<AcpiRoot>(this);
        if (!file) {
            return OsStatusOutOfMemory;
        }

        *handle = file;
        return OsStatusSuccess;
    }

    return OsStatusInterfaceNotSupported;
}

OsStatus dev::AcpiRoot::identify(OsIdentifyInfo *info) {
    detail::IdentifyAcpiRoot(mTables, info);
    return OsStatusSuccess;
}

OsStatus dev::AcpiRoot::stat(vfs2::NodeStat *stat) {
    const acpi::RsdpLocator *rsdp = mTables->locator();
    km::VirtualRange range = km::VirtualRange::of((void*)rsdp, sizeof(acpi::RsdpLocator));

    *stat = vfs2::NodeStat {
        .logical = range.size(),
        .blksize = 1,
        .blocks = range.size(),
        .access = vfs2::Access::R,
    };

    return OsStatusSuccess;
}

OsStatus dev::AcpiRoot::read(vfs2::ReadRequest request, vfs2::ReadResult *result) {
    const acpi::RsdpLocator *rsdp = mTables->locator();
    km::VirtualRange range = km::VirtualRange::of((void*)rsdp, sizeof(acpi::RsdpLocator));
    return detail::ReadTableData(range, request, result);
}

//
// smbios table
//

dev::SmBiosTable::SmBiosTable(const km::smbios::StructHeader *header, const km::SmBiosTables *tables)
    : mHeader(header)
    , mTables(tables)
{ }

OsStatus dev::SmBiosTable::query(sm::uuid uuid, const void *, size_t, vfs2::IHandle **handle) {
    if (uuid == kOsIdentifyGuid) {
        auto *identify = new vfs2::TIdentifyHandle<SmBiosTable>(this);
        if (!identify) {
            return OsStatusOutOfMemory;
        }

        *handle = identify;
        return OsStatusSuccess;
    }

    if (uuid == kOsFileGuid) {
        auto *file = new vfs2::TFileHandle<SmBiosTable>(this);
        if (!file) {
            return OsStatusOutOfMemory;
        }

        *handle = file;
        return OsStatusSuccess;
    }

    return OsStatusInterfaceNotSupported;
}

OsStatus dev::SmBiosTable::identify(OsIdentifyInfo *info) {
    detail::IdentifySmbTable(mHeader, mTables, info);
    return OsStatusSuccess;
}

OsStatus dev::SmBiosTable::stat(vfs2::NodeStat *stat) {
    km::VirtualRange range = km::VirtualRange::of((void*)mHeader, km::smbios::GetStructSize(mHeader));

    *stat = vfs2::NodeStat {
        .logical = range.size(),
        .blksize = 1,
        .blocks = range.size(),
        .access = vfs2::Access::R,
    };

    return OsStatusSuccess;
}

OsStatus dev::SmBiosTable::read(vfs2::ReadRequest request, vfs2::ReadResult *result) {
    km::VirtualRange range = km::VirtualRange::of((void*)mHeader, km::smbios::GetStructSize(mHeader));
    return detail::ReadTableData(range, request, result);
}

//
// smbios root table
//

dev::SmBiosRoot::SmBiosRoot(const km::SmBiosTables *tables)
    : mTables(tables)
{
    for (const km::smbios::StructHeader *header : *mTables) {
        dev::SmBiosTable *table = new dev::SmBiosTable(header, mTables);
        auto handle = km::format(km::Hex(header->handle).pad(4));
        mknode(vfs2::VfsStringView(handle), table);
    }
}

OsStatus dev::SmBiosRoot::query(sm::uuid uuid, const void *, size_t, vfs2::IHandle **handle) {
    if (uuid == kOsIdentifyGuid) {
        auto *identify = new vfs2::TIdentifyHandle<SmBiosRoot>(this);
        if (!identify) {
            return OsStatusOutOfMemory;
        }

        *handle = identify;
        return OsStatusSuccess;
    }

    if (uuid == kOsFolderGuid) {
        auto *folder = new vfs2::TFolderHandle<SmBiosRoot>(this);
        if (!folder) {
            return OsStatusOutOfMemory;
        }

        *handle = folder;
        return OsStatusSuccess;
    }

    if (uuid == kOsFileGuid) {
        auto *file = new vfs2::TFileHandle<SmBiosRoot>(this);
        if (!file) {
            return OsStatusOutOfMemory;
        }

        *handle = file;
        return OsStatusSuccess;
    }

    return OsStatusInterfaceNotSupported;
}

OsStatus dev::SmBiosRoot::identify(OsIdentifyInfo *info) {
    detail::IdentifySmbRoot(mTables, info);
    return OsStatusSuccess;
}

OsStatus dev::SmBiosRoot::stat(vfs2::NodeStat *stat) {
    km::VirtualRange tables = mTables->tables;

    *stat = vfs2::NodeStat {
        .logical = tables.size(),
        .blksize = 1,
        .blocks = tables.size(),
        .access = vfs2::Access::R,
    };

    return OsStatusSuccess;
}

OsStatus dev::SmBiosRoot::read(vfs2::ReadRequest request, vfs2::ReadResult *result) {
    return detail::ReadTableData(mTables->tables, request, result);
}
