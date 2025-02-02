#pragma once

#include "drivers/block/driver.hpp"
#include "std/vector.hpp"
#include "util/signature.hpp"
#include "util/uuid.hpp"

#include <cstdint>

#include <array>
#include <new>
#include <utility>

namespace km {
    class IFsDriver {
    public:
        virtual ~IFsDriver() = default;

        void operator delete(IFsDriver*, std::destroying_delete_t) {
            std::unreachable();
        }
    };

    struct [[gnu::packed]] PartitionEntry {
        uint8_t status;
        uint8_t chsBegin[3];
        uint8_t type;
        uint8_t chsEnd[3];
        uint32_t begin;
        uint32_t sectors;
    };

    static_assert(sizeof(PartitionEntry) == 16);

    struct [[gnu::packed]] MasterBootRecord {
        std::byte boot[446];
        PartitionEntry partitions[4];
        uint8_t signature[2];
    };

    static_assert(sizeof(MasterBootRecord) == 512);

    static constexpr sm::uuid kEfiSystemPartition = sm::uuid::of("C12A7328-F81F-11D2-BA4B-00A0C93EC93B");

    struct [[gnu::packed]] GptHeader {
        static constexpr std::array<char, 8> kSignature = util::Signature("EFI PART");
        std::array<char, 8> signature;
        uint32_t revision;
        uint32_t size;
        uint32_t checksum;
        uint8_t reserved[4];
        uint64_t lbaCurrent;
        uint64_t lbaBackup;
        uint64_t lbaPartitionBegin;
        uint64_t lbaParitionEnd;
        sm::uuid guid;
        uint64_t lbaPartitionArray;
        uint32_t partitionCount;
        uint32_t partitionEntrySize;
        uint32_t partitionArrayChecksum;
    };

    static_assert(sizeof(GptHeader) == 92);

    struct GptEntry {
        sm::uuid type;
        sm::uuid guid;
        uint64_t lbaBegin;
        uint64_t lbaEnd;
        uint64_t flags;
        std::array<char16_t, 36> name;
    };

    static_assert(sizeof(GptEntry) == 128);

    struct DrivePartitions {
        GptHeader header;
        stdx::Vector<GptEntry> entries;
    };

    std::optional<DrivePartitions> ParsePartitionTable(DriveMedia& drive);
}
