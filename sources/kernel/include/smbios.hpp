#pragma once

#include <bezos/status.h>

#include "memory/range.hpp"
#include "memory/vmm_heap.hpp"
#include "std/string_view.hpp"
#include "util/signature.hpp"

namespace km {
    class AddressSpace;

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

        constexpr stdx::StringView tableName(StructType type) {
            switch (type) {
            case StructType::eFirmwareInfo: return "Firmware Information";
            case StructType::eSystemInfo: return "System Information";
            case StructType::eSystemEnclosure: return "System Enclosure";
            case StructType::eProcessor: return "Processor Information";
            case StructType::eSystemSlots: return "System Slots";
            case StructType::eMemoryDevice: return "Memory Device";
            case StructType::eBootInfo: return "Boot Information";
            default: return "Unknown";
            }
        }

        namespace detail {
            stdx::StringView getStringEntry(const StructHeader *header, uint8_t string);
        }

        template<typename T>
        stdx::StringView getStringEntry(const T *entry, uint8_t string) {
            return detail::getStringEntry(reinterpret_cast<const StructHeader*>(entry), string);
        }

        size_t getStructSize(const StructHeader *header);

        // TODO: the firmware table can have extended fields
        static_assert(sizeof(FirmwareInfo) == 18);
        static_assert(sizeof(SystemInfo) == 27);

        static constexpr stdx::StringView kOracleVirtualBox = "innotek GmbH";
    }

    struct SmBiosLoadOptions {
        sm::PhysicalAddress smbios32Address;
        sm::PhysicalAddress smbios64Address;
        bool ignoreChecksum;
        bool ignore32BitEntry;
        bool ignore64BitEntry;
    };

    class SmBiosIterator {
        size_t mSize;
        const char *mAddress;

        const char *next(const char *address);

    public:
        SmBiosIterator(const void *address)
            : mSize(0)
            , mAddress((char*)address)
        { }

        const smbios::StructHeader *operator*() const;

        SmBiosIterator& operator++();

        constexpr bool operator!=(const SmBiosIterator& other) const {
            return mAddress < other.mAddress;
        }
    };

    struct SmBiosTables {
        const smbios::Entry32 *entry32;
        const smbios::Entry64 *entry64;
        VirtualRangeEx tables;
        VmemAllocation entry32Allocation;
        VmemAllocation entry64Allocation;

        SmBiosIterator begin() const {
            return SmBiosIterator(tables.front);
        }

        SmBiosIterator end() const {
            return SmBiosIterator(tables.back);
        }

        size_t size() const { return tables.size(); }

        const smbios::FirmwareInfo *firmwareInfo() const;
        const smbios::SystemInfo *systemInfo() const;

        bool isOracleVirtualBox() const;
    };

    [[nodiscard]]
    OsStatus findSmbiosTables(SmBiosLoadOptions options, km::AddressSpace& memory, SmBiosTables *tables [[outparam]]);
}
