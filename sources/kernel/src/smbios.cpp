#include "smbios.hpp"

#include "log.hpp"

#include "std/static_vector.hpp"
#include "util/defer.hpp"
#include "util/signature.hpp"

#include <stdint.h>

using namespace stdx::literals;

/// @brief Structures defined by the DMTF SMBIOS specification.
///
/// @cite DSP0134
namespace smbios {
    struct [[gnu::packed]] Entry64 {
        static constexpr auto kAnchor0 = util::Signature("_SM3_");

        std::array<char, 5> anchor;
        uint8_t checksum;
        uint8_t length;
        uint8_t major;
        uint8_t minor;
        uint8_t revision;
        uint8_t entryRevision;
        uint8_t reserved0[1];
        uint32_t tableSize;
        uint64_t tableAddress;
    };

    struct [[gnu::packed]] Entry32 {
        static constexpr auto kAnchor0 = util::Signature("_SM_");
        static constexpr auto kAnchor1 = util::Signature("_DMI_");

        std::array<char, 4> anchor0;
        uint8_t checksum0;

        uint8_t length;
        uint8_t major;
        uint8_t minor;
        uint16_t entrySize;
        uint8_t entryRevision;
        uint8_t reserved0[5];

        std::array<char, 5> anchor1;
        uint8_t checksum1;

        uint16_t tableSize;
        uint32_t tableAddress;
        uint16_t entryCount;
        uint8_t bcdRevision;
    };

    enum class StructType : uint8_t {
        eFirmwareInfo = 0,
        eSystemInfo = 1,
        eSystemEnclosure = 3,
        eProcessor = 4,
        eSystemSlots = 9,
        eMemoryDevice = 17,
        eBootInfo = 32,
    };

    struct [[gnu::packed]] StructHeader {
        smbios::StructType type;
        uint8_t length;
        uint16_t handle;
    };

    struct [[gnu::packed]] FirmwareInfo {
        StructHeader header;
        uint8_t vendor;
        uint8_t version;
        uint16_t start;
        uint8_t build;
        uint8_t romSize;
        uint64_t characteristics;
        uint8_t reserved0[1];
    };

    struct [[gnu::packed]] SystemInfo {
        StructHeader header;
        uint8_t manufacturer;
        uint8_t productName;
        uint8_t version;
        uint8_t serialNumber;
        uint8_t uuid[16];
        uint8_t wakeUpType;
        uint8_t skuNumber;
        uint8_t family;
    };
}

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

bool km::PlatformInfo::isOracleVirtualBox() const {
    return vendor == kOracleVirtualBox;
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

static OsStatus ReadSmbios64(km::PhysicalAddress address, bool ignoreChecksum, km::SystemMemory& memory, km::PlatformInfo *info) {
    const auto *smbios = memory.mapConst<smbios::Entry64>(address);
    defer { memory.unmap((void*)smbios, sizeof(smbios::Entry64)); };

    if (smbios->anchor != smbios::Entry64::kAnchor0) {
        KmDebugMessage("[SMBIOS] Invalid anchor: ", smbios->anchor, "\n");
        return OsStatusInvalidData;
    }

    std::span<const uint8_t> bytes = std::span(reinterpret_cast<const uint8_t*>(smbios), sizeof(smbios::Entry64));
    if (!TestEntryChecksum(bytes, "SMBIOS64", ignoreChecksum)) {
        return OsStatusChecksumError;
    }

    auto [ptr, end] = SmBiosMapTable(smbios, memory);

    km::PlatformInfo result{};

    while (ptr < end) {
        ptr = ReadSmbiosEntry(result, ptr);
    }

    *info = result;
    return OsStatusSuccess;
}

static OsStatus ReadSmbios32(km::PhysicalAddress address, bool ignoreChecksum, km::SystemMemory& memory, km::PlatformInfo *info) {
    const auto *smbios = memory.mapConst<smbios::Entry32>(address);
    defer { memory.unmap((void*)smbios, sizeof(smbios::Entry32)); };

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

    auto [ptr, end] = SmBiosMapTable(smbios, memory);

    km::PlatformInfo result;

    while (ptr < end) {
        ptr = ReadSmbiosEntry(result, ptr);
    }

    *info = result;
    return OsStatusSuccess;
}

OsStatus km::ReadSmbiosTables(SmBiosLoadOptions options, km::SystemMemory& memory, PlatformInfo *info [[gnu::nonnull]]) {
    //
    // Store the status as a local so that we can return the last error if both fail.
    //
    OsStatus status = OsStatusNotFound;

    if (options.smbios64Address != nullptr && !options.ignore64BitEntry) {
        status = ReadSmbios64(options.smbios64Address, options.ignoreChecksum, memory, info);
        if (status == OsStatusSuccess) {
            return status;
        }
    }

    if (options.smbios32Address != nullptr && !options.ignore32BitEntry) {
        status = ReadSmbios32(options.smbios32Address, options.ignoreChecksum, memory, info);
        if (status == OsStatusSuccess) {
            return status;
        }
    }

    return OsStatusNotFound;
}
