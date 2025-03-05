#include "devices/sysfs.hpp"

#include "smbios.hpp"

dev::AcpiTable::AcpiTable(std::span<const uint8_t> table)
    : mTable(table)
{
    const acpi::RsdtHeader *header = reinterpret_cast<const acpi::RsdtHeader*>(mTable.data());
    mIdentifyInfo = OsIdentifyInfo {
        .DriverVendor = "BezOS",
        .DriverVersion = OS_VERSION(1, 0, 0),
    };

    memcpy(mIdentifyInfo.DisplayName, header->signature.data(), 4);
    memcpy(mIdentifyInfo.Model, header->tableId, 8);
    memcpy(mIdentifyInfo.DeviceVendor, header->oemid, 6);
}

dev::SmBiosTable::SmBiosTable(const km::smbios::StructHeader *header)
    : mHeader(header)
{ }

dev::SmBiosTables::SmBiosTables(const km::SmBiosTables *tables)
    : mTables(tables)
{
    mIdentifyInfo = OsIdentifyInfo {
        .DriverVendor = "BezOS",
        .DriverVersion = OS_VERSION(1, 0, 0),
    };

    for (const km::smbios::StructHeader *header : *mTables) {
        dev::SmBiosTable *table = new dev::SmBiosTable(header);
        auto handle = km::format(km::Hex(header->handle).pad(4));
        addNode(vfs2::VfsString(handle), table);
    }
}
