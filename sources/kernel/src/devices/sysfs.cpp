#include "devices/sysfs.hpp"

#include "acpi/acpi.hpp"
#include "acpi/header.hpp"
#include "smbios.hpp"

namespace smbios = km::smbios;

namespace stdr = std::ranges;

dev::AcpiTable::AcpiTable(const acpi::RsdtHeader *table)
    : mHeader(table)
{
    mIdentifyInfo = OsIdentifyInfo {
        .DriverVendor = "BezOS",
        .DriverVersion = OS_VERSION(1, 0, 0),
    };

    stdr::copy(mHeader->signature, mIdentifyInfo.DisplayName);
    stdr::copy(mHeader->oemid, mIdentifyInfo.DeviceVendor);
    stdr::copy(mHeader->tableId, mIdentifyInfo.Model);

    if (mHeader->revision == 0) {
        stdr::copy("1.0", mIdentifyInfo.FirmwareRevision);
    } else {
        stdr::copy("2.0", mIdentifyInfo.FirmwareRevision);
    }
}

dev::AcpiRoot::AcpiRoot(const acpi::AcpiTables *tables)
    : mTables(tables)
{
    installFolderInterface();

    const auto *rsdp = mTables->locator();

    mIdentifyInfo = OsIdentifyInfo {
        .DisplayName = "ACPI",
        .DriverVendor = "BezOS",
        .DriverVersion = OS_VERSION(1, 0, 0),
    };

    stdr::copy(rsdp->oemid, mIdentifyInfo.DeviceVendor);

    if (rsdp->revision == 0) {
        stdr::copy("1.0", mIdentifyInfo.FirmwareRevision);
    } else {
        stdr::copy("2.0", mIdentifyInfo.FirmwareRevision);
    }

    for (const acpi::RsdtHeader *header : mTables->entries()) {
        dev::AcpiTable *table = new dev::AcpiTable(header);
        addNode(vfs2::VfsString(header->signature), table);
    }
}

dev::SmBiosTable::SmBiosTable(const km::smbios::StructHeader *header, const OsIdentifyInfo& info)
    : mHeader(header)
{
    mIdentifyInfo = OsIdentifyInfo {
        .DriverVersion = info.DriverVersion,
    };

    stdr::copy(info.FirmwareRevision, mIdentifyInfo.FirmwareRevision);
    stdr::copy(info.DeviceVendor, mIdentifyInfo.DeviceVendor);
    stdr::copy(info.DriverVendor, mIdentifyInfo.DriverVendor);
    stdr::copy(info.Model, mIdentifyInfo.Model);

    auto tableName = km::smbios::TableName(header->type);
    stdr::copy(tableName, mIdentifyInfo.DisplayName);

    auto handle = km::format(km::Hex(header->handle).pad(4));
    stdr::copy(handle, mIdentifyInfo.Serial);
}

dev::SmBiosTables::SmBiosTables(const km::SmBiosTables *tables)
    : mTables(tables)
{
    installFolderInterface();

    const auto *firmware = mTables->firmwareInfo();
    const auto *system = mTables->systemInfo();

    stdx::StringView vendor = smbios::GetStringEntry(firmware, firmware->vendor);
    stdx::StringView version = smbios::GetStringEntry(firmware, firmware->version);
    stdx::StringView manufacturer = smbios::GetStringEntry(system, system->manufacturer);
    stdx::StringView product = smbios::GetStringEntry(system, system->productName);
    stdx::StringView serial = smbios::GetStringEntry(system, system->serialNumber);

    mIdentifyInfo = OsIdentifyInfo {
        .DriverVendor = "BezOS",
        .DriverVersion = OS_VERSION(1, 0, 0),
    };

    stdr::copy(vendor, mIdentifyInfo.DisplayName);
    stdr::copy(product, mIdentifyInfo.Model);
    stdr::copy(serial, mIdentifyInfo.Serial);
    stdr::copy(manufacturer, mIdentifyInfo.DeviceVendor);
    stdr::copy(version, mIdentifyInfo.FirmwareRevision);

    for (const km::smbios::StructHeader *header : *mTables) {
        dev::SmBiosTable *table = new dev::SmBiosTable(header, mIdentifyInfo);
        auto handle = km::format(km::Hex(header->handle).pad(4));
        addNode(vfs2::VfsString(handle), table);
    }
}
