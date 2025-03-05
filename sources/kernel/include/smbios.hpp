#pragma once

#include "memory.hpp"
#include "std/static_string.hpp"
#include "util/signature.hpp"

namespace km {
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

    struct PlatformInfo {
        static constexpr stdx::StringView kOracleVirtualBox = "innotek GmbH";

        using BiosString = stdx::StaticString<64>;

        /// @brief The vendor of the BIOS.
        BiosString vendor;

        /// @brief The version of the BIOS.
        BiosString version;

        BiosString manufacturer;
        BiosString product;
        BiosString serial;

        bool isOracleVirtualBox() const;
    };

    struct SmBiosLoadOptions {
        PhysicalAddress smbios32Address;
        PhysicalAddress smbios64Address;
        bool ignoreChecksum;
        bool ignore32BitEntry;
        bool ignore64BitEntry;
    };

    class SmBiosIterator {
        const char *mAddress;

    public:
        SmBiosIterator(const void *address)
            : mAddress((char*)address)
        { }

        const smbios::StructHeader *header() const {
            return reinterpret_cast<const smbios::StructHeader*>(mAddress);
        }

        const smbios::StructHeader *operator*() const {
            return header();
        }

        SmBiosIterator& operator++() {
            const auto *info = header();
            mAddress += info->length;
            return *this;
        }

        constexpr bool operator!=(const SmBiosIterator& other) const {
            return mAddress >= other.mAddress;
        }
    };

    struct SmBiosTables {
        const smbios::Entry32 *entry32;
        const smbios::Entry64 *entry64;
        VirtualRange tables;

        SmBiosIterator begin() const {
            return SmBiosIterator(tables.front);
        }

        SmBiosIterator end() const {
            return SmBiosIterator(tables.back);
        }
    };

    [[nodiscard]]
    OsStatus ReadSmbiosTables(SmBiosLoadOptions options, km::SystemMemory& memory, PlatformInfo *info [[gnu::nonnull]]);

    [[nodiscard]]
    OsStatus FindSmbiosTables(SmBiosLoadOptions options, km::SystemMemory& memory, SmBiosTables *tables [[gnu::nonnull]]);
}
