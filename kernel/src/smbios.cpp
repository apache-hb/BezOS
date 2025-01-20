#include "smbios.hpp"

#include "log.hpp"

using namespace stdx::literals;

bool PlatformInfo::isOracleVirtualBox() const {
    return vendor == "innotek GmbH"_sv;
}

/// @brief Read an SMBIOS entry from the given pointer.
///
/// @param info The platform info to write the entry to.
/// @param ptr The pointer to read the entry from.
/// @return The start of the next entry or NULL if the entry is invalid.
static const void *KmReadSmbiosEntry(PlatformInfo& info, const void *ptr) {
    const SmBiosEntryHeader *header = (const SmBiosEntryHeader*)ptr;
    KmDebugMessage("[SMBIOS] Type: ", header->type, ", Length: ", header->length, ", Handle: ", header->handle, "\n");

    const char *front = (const char*)((uintptr_t)ptr + header->length);
    const char *back = front;

    stdx::StaticVector<stdx::StringView, 16> strings;

    auto getString = [&](uint8_t index) -> stdx::StringView {
        if (index == 0)
            return "Not specified"_sv;

        if (index >= strings.count())
            return "Invalid index"_sv;

        return strings[index - 1];
    };

    while (true) {
        while (*back != '\0') {
            back++;
        }

        if (*(back + 1) == '\0') {
            break;
        }

        stdx::StringView entry = stdx::StringView(front, back);
        strings.add(entry);

        back++;
        front = back;
    }

    if (header->type == 0) {
        const SmBiosFirmwareInfo *firmware = (const SmBiosFirmwareInfo*)ptr;
        info.vendor = getString(firmware->vendor);
        info.version = getString(firmware->version);
    } else if (header->type == 1) {
        const SmBiosSystemInfo *system = (const SmBiosSystemInfo*)ptr;
        info.manufacturer = getString(system->manufacturer);
        info.product = getString(system->productName);
        info.serial = getString(system->serialNumber);
    }

    return back + 2;
}

static PlatformInfo KmReadSmbios64(km::PhysicalAddress address, km::SystemMemory& memory) {
    const SmBiosHeader64 *smbios = memory.mapObject<SmBiosHeader64>(address);

    void *tableAddress = memory.map(smbios->tableAddress, smbios->tableAddress + smbios->tableSize);
    KmDebugMessage("[SMBIOS] Table address: ", km::Hex(smbios->tableAddress).pad(16, '0'), ", Size: ", smbios->tableSize, "\n");
    KmDebugMessage(km::HexDump(std::span(reinterpret_cast<const uint8_t*>(tableAddress), smbios->tableSize)), "\n");

    const void *ptr = tableAddress;
    const void *end = (const void*)((uintptr_t)ptr + smbios->tableSize);

    PlatformInfo info;

    while (ptr < end) {
        ptr = KmReadSmbiosEntry(info, ptr);
    }

    return info;
}

static PlatformInfo KmReadSmbios32(km::PhysicalAddress address, km::SystemMemory& memory) {
    const SmBiosHeader32 *smbios = memory.mapObject<SmBiosHeader32>(address);

    void *tableAddress = memory.map(smbios->tableAddress, smbios->tableAddress + smbios->tableSize);
    KmDebugMessage("[SMBIOS] Table address: ", km::Hex(smbios->tableAddress).pad(8, '0'), ", Size: ", smbios->tableSize, "\n");
    KmDebugMessage(km::HexDump(std::span(reinterpret_cast<const uint8_t*>(tableAddress), smbios->tableSize)), "\n");

    const void *ptr = tableAddress;
    const void *end = (const void*)((uintptr_t)ptr + smbios->tableSize);

    PlatformInfo info;

    while (ptr < end) {
        ptr = KmReadSmbiosEntry(info, ptr);
    }

    return info;
}

PlatformInfo KmGetPlatformInfo(const KernelLaunch& launch, km::SystemMemory& memory) {
    if (launch.smbios64Address != nullptr) {
        return KmReadSmbios64(launch.smbios64Address, memory);
    } else if (launch.smbios32Address != nullptr) {
        return KmReadSmbios32(launch.smbios32Address, memory);
    }

    return PlatformInfo { };
}
