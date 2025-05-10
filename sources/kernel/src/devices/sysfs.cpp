#include "devices/sysfs.hpp"

#include "acpi/acpi.hpp"
#include "acpi/header.hpp"
#include "fs/file.hpp"
#include "fs/identify.hpp"
#include "fs/iterator.hpp"
#include "fs/query.hpp"
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

OsStatus dev::detail::ReadTableData(km::VirtualRangeEx tables, vfs::ReadRequest request, vfs::ReadResult *result) {
    uint64_t front = request.offset;
    uint64_t back = request.offset + request.size();

    if (front >= tables.size()) {
        return OsStatusEndOfFile;
    }

    if (back > tables.size()) {
        back = tables.size();
    }

    size_t count = back - front;
    memcpy(request.begin, tables.front + front, count);
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

static constexpr inline vfs::InterfaceList kAcpiTableInterfaceList = std::to_array({
    vfs::InterfaceOf<vfs::TIdentifyHandle<dev::AcpiTable>, dev::AcpiTable>(kOsIdentifyGuid),
    vfs::InterfaceOf<vfs::TFileHandle<dev::AcpiTable>, dev::AcpiTable>(kOsFileGuid),
});

static constexpr inline vfs::InterfaceList kAcpiRootInterfaceList = std::to_array({
    vfs::InterfaceOf<vfs::TIdentifyHandle<dev::AcpiRoot>, dev::AcpiRoot>(kOsIdentifyGuid),
    vfs::InterfaceOf<vfs::TFileHandle<dev::AcpiRoot>, dev::AcpiRoot>(kOsFileGuid),
    vfs::InterfaceOf<vfs::TFolderHandle<dev::AcpiRoot>, dev::AcpiRoot>(kOsFolderGuid),
    vfs::InterfaceOf<vfs::TIteratorHandle<dev::AcpiRoot>, dev::AcpiRoot>(kOsIteratorGuid),
});

static constexpr inline vfs::InterfaceList kSmbTableInterfaceList = std::to_array({
    vfs::InterfaceOf<vfs::TIdentifyHandle<dev::SmBiosTable>, dev::SmBiosTable>(kOsIdentifyGuid),
    vfs::InterfaceOf<vfs::TFileHandle<dev::SmBiosTable>, dev::SmBiosTable>(kOsFileGuid),
});

static constexpr inline vfs::InterfaceList kSmbRootInterfaceList = std::to_array({
    vfs::InterfaceOf<vfs::TIdentifyHandle<dev::SmBiosRoot>, dev::SmBiosRoot>(kOsIdentifyGuid),
    vfs::InterfaceOf<vfs::TFileHandle<dev::SmBiosRoot>, dev::SmBiosRoot>(kOsFileGuid),
    vfs::InterfaceOf<vfs::TFolderHandle<dev::SmBiosRoot>, dev::SmBiosRoot>(kOsFolderGuid),
    vfs::InterfaceOf<vfs::TIteratorHandle<dev::SmBiosRoot>, dev::SmBiosRoot>(kOsIteratorGuid),
});

dev::AcpiTable::AcpiTable(const acpi::RsdtHeader *table)
    : mHeader(table)
{ }

OsStatus dev::AcpiTable::query(sm::uuid uuid, const void *data, size_t size, vfs::IHandle **handle) {
    return kAcpiTableInterfaceList.query(loanShared(), uuid, data, size, handle);
}

OsStatus dev::AcpiTable::interfaces(OsIdentifyInterfaceList *list) {
    return kAcpiTableInterfaceList.list(list);
}

OsStatus dev::AcpiTable::identify(OsIdentifyInfo *info) {
    detail::IdentifyAcpiTable(mHeader, info);
    return OsStatusSuccess;
}

OsStatus dev::AcpiTable::stat(OsFileInfo *stat) {
    km::VirtualRange range = km::VirtualRange::of((void*)mHeader, mHeader->length);

    *stat = OsFileInfo {
        .LogicalSize = range.size(),
        .BlockSize = 1,
        .BlockCount = range.size(),
    };

    stdr::copy(mHeader->signature, stat->Name);

    return OsStatusSuccess;
}

OsStatus dev::AcpiTable::read(vfs::ReadRequest request, vfs::ReadResult *result) {
    km::VirtualRangeEx range = km::VirtualRangeEx::of((void*)mHeader, mHeader->length);
    return detail::ReadTableData(range, request, result);
}

//
// acpi locator
//

dev::AcpiRoot::AcpiRoot(const acpi::AcpiTables *tables)
    : mTables(tables)
{ }

OsStatus dev::AcpiRoot::query(sm::uuid uuid, const void *data, size_t size, vfs::IHandle **handle) {
    return kAcpiRootInterfaceList.query(loanShared(), uuid, data, size, handle);
}

OsStatus dev::AcpiRoot::interfaces(OsIdentifyInterfaceList *list) {
    return kAcpiRootInterfaceList.list(list);
}

OsStatus dev::AcpiRoot::identify(OsIdentifyInfo *info) {
    detail::IdentifyAcpiRoot(mTables, info);
    return OsStatusSuccess;
}

OsStatus dev::AcpiRoot::stat(OsFileInfo *stat) {
    const acpi::RsdpLocator *rsdp = mTables->locator();
    km::VirtualRange range = km::VirtualRange::of((void*)rsdp, sizeof(acpi::RsdpLocator));

    *stat = OsFileInfo {
        .Name = "ACPI",
        .LogicalSize = range.size(),
        .BlockSize = 1,
        .BlockCount = range.size(),
    };

    return OsStatusSuccess;
}

OsStatus dev::AcpiRoot::read(vfs::ReadRequest request, vfs::ReadResult *result) {
    const acpi::RsdpLocator *rsdp = mTables->locator();
    km::VirtualRangeEx range = km::VirtualRangeEx::of((void*)rsdp, sizeof(acpi::RsdpLocator));
    return detail::ReadTableData(range, request, result);
}

sm::RcuSharedPtr<dev::AcpiRoot> dev::AcpiRoot::create(sm::RcuDomain *domain, const acpi::AcpiTables *tables) {
    auto root = sm::rcuMakeShared<dev::AcpiRoot>(domain, tables);
    if (root == nullptr) {
        return nullptr;
    }

    for (const acpi::RsdtHeader *header : tables->entries()) {
        auto table = sm::rcuMakeShared<dev::AcpiTable>(domain, header);
        root->mknode(root, vfs::VfsStringView(header->signature), table);
    }

    return root;
}

//
// smbios table
//

dev::SmBiosTable::SmBiosTable(const km::smbios::StructHeader *header, const km::SmBiosTables *tables)
    : mHeader(header)
    , mTables(tables)
{ }


OsStatus dev::SmBiosTable::query(sm::uuid uuid, const void *data, size_t size, vfs::IHandle **handle) {
    return kSmbTableInterfaceList.query(loanShared(), uuid, data, size, handle);
}

OsStatus dev::SmBiosTable::interfaces(OsIdentifyInterfaceList *list) {
    return kSmbTableInterfaceList.list(list);
}

OsStatus dev::SmBiosTable::identify(OsIdentifyInfo *info) {
    detail::IdentifySmbTable(mHeader, mTables, info);
    return OsStatusSuccess;
}

OsStatus dev::SmBiosTable::stat(OsFileInfo *stat) {
    km::VirtualRange range = km::VirtualRange::of((void*)mHeader, km::smbios::GetStructSize(mHeader));

    *stat = OsFileInfo {
        .LogicalSize = range.size(),
        .BlockSize = 1,
        .BlockCount = range.size(),
    };

    auto name = km::smbios::TableName(mHeader->type);
    stdr::copy(name, stat->Name);

    return OsStatusSuccess;
}

OsStatus dev::SmBiosTable::read(vfs::ReadRequest request, vfs::ReadResult *result) {
    km::VirtualRangeEx range = km::VirtualRangeEx::of((void*)mHeader, km::smbios::GetStructSize(mHeader));
    return detail::ReadTableData(range, request, result);
}

//
// smbios root table
//

dev::SmBiosRoot::SmBiosRoot(const km::SmBiosTables *tables)
    : mTables(tables)
{ }

OsStatus dev::SmBiosRoot::query(sm::uuid uuid, const void *data, size_t size, vfs::IHandle **handle) {
    return kSmbRootInterfaceList.query(loanShared(), uuid, data, size, handle);
}

OsStatus dev::SmBiosRoot::interfaces(OsIdentifyInterfaceList *list) {
    return kSmbRootInterfaceList.list(list);
}

OsStatus dev::SmBiosRoot::identify(OsIdentifyInfo *info) {
    detail::IdentifySmbRoot(mTables, info);
    return OsStatusSuccess;
}

OsStatus dev::SmBiosRoot::stat(OsFileInfo *stat) {
    km::VirtualRangeEx tables = mTables->tables;

    *stat = OsFileInfo {
        .Name = "SMBIOS",
        .LogicalSize = tables.size(),
        .BlockSize = 1,
        .BlockCount = tables.size(),
    };

    return OsStatusSuccess;
}

OsStatus dev::SmBiosRoot::read(vfs::ReadRequest request, vfs::ReadResult *result) {
    return detail::ReadTableData(mTables->tables, request, result);
}

sm::RcuSharedPtr<dev::SmBiosRoot> dev::SmBiosRoot::create(sm::RcuDomain *domain, const km::SmBiosTables *tables) {
    auto root = sm::rcuMakeShared<dev::SmBiosRoot>(domain, tables);
    if (root == nullptr) {
        return nullptr;
    }

    for (const km::smbios::StructHeader *header : *tables) {
        auto table = sm::rcuMakeShared<dev::SmBiosTable>(domain, header, tables);
        auto handle = km::format(km::Hex(header->handle).pad(4));
        root->mknode(root, vfs::VfsStringView(handle), table);
    }

    return root;
}
