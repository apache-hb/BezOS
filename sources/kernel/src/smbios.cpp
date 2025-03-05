#include "smbios.hpp"

#include "log.hpp"

#include "std/static_vector.hpp"
#include "util/defer.hpp"

#include <stdint.h>

using namespace stdx::literals;

namespace smbios = km::smbios;

template<typename T>
constexpr km::MemoryRange SmBiosTableRange(const T *table) {
    return km::MemoryRange::of(table->tableAddress, table->tableSize);
}

template<typename T>
static km::VirtualRange SmBiosMapTable(const T *table, km::SystemMemory& memory) {
    KmDebugMessage("[SMBIOS] Table: ", (void*)table, "\n");
    KmDebugMessage(km::HexDump(std::span(reinterpret_cast<const uint8_t*>(table), sizeof(T))), "\n");

    void *address = memory.map(SmBiosTableRange(table));
    KmDebugMessage("[SMBIOS] Table address: ", km::Hex(table->tableAddress).pad(sizeof(T::tableAddress) * 2), ", Size: ", auto{table->tableSize}, "\n");
    KmDebugMessage(km::HexDump(std::span(reinterpret_cast<const uint8_t*>(address), table->tableSize)), "\n");
    return km::VirtualRange::of(address, table->tableSize);
}

constexpr bool TestEntryChecksum(std::span<const uint8_t> bytes, stdx::StringView name, bool ignoreChecksum) {
    uint8_t sum = 0;
    for (uint8_t byte : bytes) {
        sum += byte;
    }

    if (sum != 0) {
        KmDebugMessage("[SMBIOS] Invalid checksum for ", name, ": ", sum, "\n");

        if (ignoreChecksum) {
            KmDebugMessage("[SMBIOS] The system operator has indicated that this checksum should be ignored. continuing...\n");
            return true;
        }
    }

    return sum == 0;
}

stdx::StringView km::smbios::detail::GetStringEntry(const StructHeader *header, uint8_t string) {
    if (string == 0) {
        return "Not specified"_sv;
    }

    const char *front = (const char*)((const char*)header + header->length);
    const char *back = front;

    while (true) {
        while (*back != '\0') {
            back++;
        }

        if (*(back + 1) == '\0') {
            break;
        }

        stdx::StringView entry = stdx::StringView(front, back);

        if (--string == 0) {
            return entry;
        }

        back++;
        front = back;
    }

    return "Invalid index"_sv;
}

/// @brief Read an SMBIOS entry from the given pointer.
///
/// @param info The platform info to write the entry to.
/// @param ptr The pointer to read the entry from.
/// @return The start of the next entry.
static const void *ReadSmbiosEntry(km::PlatformInfo& info, const void *ptr) {
    const smbios::StructHeader *header = (const smbios::StructHeader*)ptr;
    KmDebugMessage("[SMBIOS] Type: ", std::to_underlying(header->type), ", Length: ", header->length, ", Handle: ", auto{header->handle}, "\n");

    const char *front = (const char*)((uintptr_t)ptr + header->length);
    const char *back = front;

    stdx::StaticVector<stdx::StringView, 16> strings;

    auto getString = [&](uint8_t index) -> stdx::StringView {
        if (index == 0)
            return "Not specified"_sv;

        if (index > strings.count())
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

    if (header->type == smbios::StructType::eFirmwareInfo) {
        const smbios::FirmwareInfo *firmware = (const smbios::FirmwareInfo*)ptr;
        info.vendor = getString(firmware->vendor);
        info.version = getString(firmware->version);
    } else if (header->type == smbios::StructType::eSystemInfo) {
        const smbios::SystemInfo *system = (const smbios::SystemInfo*)ptr;
        info.manufacturer = getString(system->manufacturer);
        info.product = getString(system->productName);
        info.serial = getString(system->serialNumber);
    }

    return back + 2;
}

static OsStatus FindSmbios64(km::PhysicalAddress address, bool ignoreChecksum, km::SystemMemory& memory, const smbios::Entry64 **entry) {
    const auto *smbios = memory.mapConst<smbios::Entry64>(address);

    if (smbios->anchor != smbios::Entry64::kAnchor0) {
        KmDebugMessage("[SMBIOS] Invalid anchor: ", smbios->anchor, "\n");
        return OsStatusInvalidData;
    }

    std::span<const uint8_t> bytes = std::span(reinterpret_cast<const uint8_t*>(smbios), sizeof(smbios::Entry64));
    if (!TestEntryChecksum(bytes, "SMBIOS64", ignoreChecksum)) {
        return OsStatusChecksumError;
    }

    *entry = smbios;
    return OsStatusSuccess;
}

static OsStatus FindSmbios32(km::PhysicalAddress address, bool ignoreChecksum, km::SystemMemory& memory, const smbios::Entry32 **entry) {
    const auto *smbios = memory.mapConst<smbios::Entry32>(address);

    if (smbios->anchor0 != smbios::Entry32::kAnchor0) {
        KmDebugMessage("[SMBIOS] Invalid anchor: ", smbios->anchor0, "\n");
        return OsStatusInvalidData;
    }

    //
    // The SMBIOS32 entry point structure is split into a base header and an extended header.
    // Each of these headers needs to be checksummed individually. The first header is 0x00:0x10
    // and the extended header is afterwards at 0x10:0x1e.
    //
    std::span<const uint8_t> bytes = std::span(reinterpret_cast<const uint8_t*>(smbios), 0x10);
    if (!TestEntryChecksum(bytes, "SMBIOS32", ignoreChecksum)) {
        return OsStatusChecksumError;
    }

    bytes = std::span(reinterpret_cast<const uint8_t*>(smbios) + 0x10, 0xf);
    if (!TestEntryChecksum(bytes, "SMBIOS32 Extended", ignoreChecksum)) {
        return OsStatusChecksumError;
    }

    *entry = smbios;
    return OsStatusSuccess;
}

OsStatus km::ReadSmbiosTables(SmBiosLoadOptions options, km::SystemMemory& memory, PlatformInfo *info [[gnu::nonnull]]) {
    SmBiosTables tables{};
    if (OsStatus status = FindSmbiosTables(options, memory, &tables)) {
        return status;
    }

    defer {
        if (!tables.tables.isEmpty()) {
            memory.unmap(tables.tables);
        }

        if (tables.entry32 != nullptr) {
            memory.unmap((void*)tables.entry32, sizeof(smbios::Entry32));
        }

        if (tables.entry64 != nullptr) {
            memory.unmap((void*)tables.entry64, sizeof(smbios::Entry64));
        }
    };

    auto [ptr, end] = tables.tables;
    km::PlatformInfo result{};

    while (ptr < end) {
        ptr = ReadSmbiosEntry(result, ptr);
    }

    *info = result;
    return OsStatusSuccess;
}

OsStatus km::FindSmbiosTables(SmBiosLoadOptions options, km::SystemMemory& memory, SmBiosTables *tables [[gnu::nonnull]]) {
    //
    // Store the status as a local so that we can return the last error if both fail.
    //
    OsStatus status = OsStatusNotFound;

    SmBiosTables result{};

    if (options.smbios64Address != nullptr && !options.ignore64BitEntry) {
        status = FindSmbios64(options.smbios64Address, options.ignoreChecksum, memory, &result.entry64);
        if (status == OsStatusSuccess) {
            result.tables = SmBiosMapTable(result.entry64, memory);
            *tables = result;
            return status;
        }
    }

    if (options.smbios32Address != nullptr && !options.ignore32BitEntry) {
        status = FindSmbios32(options.smbios32Address, options.ignoreChecksum, memory, &result.entry32);
        if (status == OsStatusSuccess) {
            result.tables = SmBiosMapTable(result.entry32, memory);
            *tables = result;
            return status;
        }
    }

    return OsStatusNotFound;
}
