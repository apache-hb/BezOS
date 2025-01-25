#include "smbios.hpp"

#include "log.hpp"

#include "std/static_vector.hpp"

using namespace stdx::literals;

bool km::PlatformInfo::isOracleVirtualBox() const {
    return vendor == "innotek GmbH"_sv;
}

/// @brief Read an SMBIOS entry from the given pointer.
///
/// @param info The platform info to write the entry to.
/// @param ptr The pointer to read the entry from.
/// @return The start of the next entry.
static const void *ReadSmbiosEntry(km::PlatformInfo& info, const void *ptr) {
    const km::SmBiosEntryHeader *header = (const km::SmBiosEntryHeader*)ptr;
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
        const km::SmBiosFirmwareInfo *firmware = (const km::SmBiosFirmwareInfo*)ptr;
        info.vendor = getString(firmware->vendor);
        info.version = getString(firmware->version);
    } else if (header->type == 1) {
        const km::SmBiosSystemInfo *system = (const km::SmBiosSystemInfo*)ptr;
        info.manufacturer = getString(system->manufacturer);
        info.product = getString(system->productName);
        info.serial = getString(system->serialNumber);
    }

    return back + 2;
}

static km::PlatformInfo ReadSmbios64(km::PhysicalAddress address, km::SystemMemory& memory) {
    const km::SmBiosHeader64 *smbios = memory.mapObject<km::SmBiosHeader64>(address);

    void *tableAddress = memory.map(smbios->tableAddress, smbios->tableAddress + smbios->tableSize);
    KmDebugMessage("[SMBIOS] Table address: ", km::Hex(smbios->tableAddress).pad(16, '0'), ", Size: ", smbios->tableSize, "\n");
    KmDebugMessage(km::HexDump(std::span(reinterpret_cast<const uint8_t*>(tableAddress), smbios->tableSize)), "\n");

    const void *ptr = tableAddress;
    const void *end = (const void*)((uintptr_t)ptr + smbios->tableSize);

    km::PlatformInfo info;

    while (ptr < end) {
        ptr = ReadSmbiosEntry(info, ptr);
    }

    return info;
}

static km::PlatformInfo ReadSmbios32(km::PhysicalAddress address, km::SystemMemory& memory) {
    const km::SmBiosHeader32 *smbios = memory.mapObject<km::SmBiosHeader32>(address);

    void *tableAddress = memory.map(smbios->tableAddress, smbios->tableAddress + smbios->tableSize);
    KmDebugMessage("[SMBIOS] Table address: ", km::Hex(smbios->tableAddress).pad(8, '0'), ", Size: ", smbios->tableSize, "\n");
    KmDebugMessage(km::HexDump(std::span(reinterpret_cast<const uint8_t*>(tableAddress), smbios->tableSize)), "\n");

    const void *ptr = tableAddress;
    const void *end = (const void*)((uintptr_t)ptr + smbios->tableSize);

    km::PlatformInfo info;

    while (ptr < end) {
        ptr = ReadSmbiosEntry(info, ptr);
    }

    return info;
}

km::PlatformInfo km::GetPlatformInfo(
    km::PhysicalAddress smbios32Address,
    km::PhysicalAddress smbios64Address,
    km::SystemMemory& memory
) {
    if (smbios64Address != nullptr) {
        return ReadSmbios64(smbios64Address, memory);
    } else if (smbios32Address != nullptr) {
        return ReadSmbios32(smbios32Address, memory);
    }

    return PlatformInfo { };
}
