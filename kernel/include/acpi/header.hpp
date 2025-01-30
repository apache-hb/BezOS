#pragma once

#include "util/format.hpp"
#include <stdint.h>

namespace acpi {
    using TableSignature = std::array<char, 4>;

    struct [[gnu::packed]] RsdtHeader {
        TableSignature signature;
        uint32_t length;
        uint8_t revision;
        uint8_t checksum;
        char oemid[6];
        char tableId[8];
        uint32_t oemRevision;
        uint32_t creatorId;
        uint32_t creatorRevision;
    };

    static_assert(sizeof(RsdtHeader) == 36);

    enum class AddressSpaceId : uint8_t {
        eSystemMemory = 0x00,
        eSystemIo = 0x01,
        ePciConfig = 0x02,
        eEmbeddedController = 0x03,
        eSmbus = 0x04,
        eSystemCmos = 0x05,
        ePciBarTarget = 0x06,
        eIpmi = 0x07,
        eGeneralPurposeIo = 0x08,
        eGenericSerialBus = 0x09,
        ePlatformCommChannel = 0x0A,
        eFunctionalFixedHardware = 0x7F,
    };

    enum class AccessSize : uint8_t {
        eUndefined = 0,
        eByte = 1,
        eWord = 2,
        eDword = 3,
        eQword = 4,
    };

    // ACPI 5.2.3.2. Generic Address Structure
    struct [[gnu::packed]] GenericAddress {
        AddressSpaceId addressSpace;
        uint8_t width;
        uint8_t offset;
        AccessSize accessSize;
        uint64_t address;
    };

    static_assert(sizeof(GenericAddress) == 12);

    template<typename T>
    concept IsAcpiTable = requires {
        { T::kSignature } -> std::convertible_to<TableSignature>;
    };

    template<IsAcpiTable T>
    const T *TableCast(const RsdtHeader *header) {
        if (header->signature == T::kSignature) {
            return reinterpret_cast<const T*>(header);
        }

        return nullptr;
    }
}

template<>
struct km::Format<acpi::AddressSpaceId> {
    using String = stdx::StaticString<32>;
    static String toString(acpi::AddressSpaceId id);
};

template<>
struct km::Format<acpi::AccessSize> {
    using String = stdx::StaticString<32>;
    static String toString(acpi::AccessSize size);
};

template<>
struct km::Format<acpi::GenericAddress> {
    using String = stdx::StaticString<128>;
    static String toString(acpi::GenericAddress addr);
};
