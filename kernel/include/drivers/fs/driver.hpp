#pragma once

#include "drivers/block/driver.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"
#include "util/signature.hpp"
#include "util/uuid.hpp"

#include <cstdint>

#include <array>
#include <expected>
#include <new>
#include <utility>

namespace km {
    class FsPath {
        stdx::String mPath;
        size_t mSegments;

        constexpr FsPath(stdx::String path, size_t segments)
            : mPath(path)
            , mSegments(segments)
        { }

    public:
        static constexpr FsPath root() {
            return FsPath("", 0);
        }
    };

    class FsFile {

    };

    class FsFolder {

    };

    class IFileSystemDriver {
        BlockDevice *mMedia;

    protected:
        IFileSystemDriver(BlockDevice *media) : mMedia(media) { }

        BlockDevice *device() const { return mMedia; }

    public:
        virtual ~IFileSystemDriver() = default;

        void operator delete(IFileSystemDriver*, std::destroying_delete_t) {
            std::unreachable();
        }
    };

    struct [[gnu::packed]] ChsAddress {
        static constexpr uint16_t kMaxCylinder = (1 << 10) - 1;

        uint8_t head;
        uint8_t body[2];

        static constexpr ChsAddress ofChs(uint8_t head, uint16_t cylinder, uint8_t sector) {
            ChsAddress result = { .head = head };
            result.setCylinder(cylinder);
            result.setSector(sector);
            return result;
        }

        constexpr uint16_t cylinder() const {
            return (uint16_t(body[0] & 0b1100'0000) << 2) | uint16_t(body[1]);
        }

        constexpr void setCylinder(uint16_t it) {
            body[0] = (body[0] & 0b0011'1111) | uint8_t((it & 0b11'0000'0000) >> 2);
            body[1] = it & 0xFF;
        }

        constexpr uint8_t sector() const {
            return body[0] & 0b0011'1111;
        }

        constexpr void setSector(uint8_t it) {
            body[0] = (body[0] & 0b1100'0000) | (it & 0b0011'1111);
        }

        constexpr bool operator==(const ChsAddress& other) const = default;
        constexpr bool operator!=(const ChsAddress& other) const = default;
    };

    static_assert(sizeof(ChsAddress) == 3);

    struct [[gnu::packed]] MbrPartition {
        uint8_t status;
        ChsAddress chsBegin;
        uint8_t type;
        ChsAddress chsEnd;
        sm::le<uint32_t> lbaBegin;
        sm::le<uint32_t> sectors;

        bool isActive() const {
            return status & (1 << 7);
        }

        bool isGpt() const {
            return type == 0xEE;
        }
    };

    static_assert(sizeof(MbrPartition) == 16);

    struct [[gnu::packed]] MasterBootRecord {
        static constexpr std::array<uint8_t, 2> kSignature = { 0x55, 0xAA };
        std::byte boot[446];
        MbrPartition partitions[4];
        std::array<uint8_t, 2> signature;

        bool isGpt() const {
            for (const auto& partition : partitions)
                if (partition.isGpt())
                    return true;

            return false;
        }
    };

    static_assert(sizeof(MasterBootRecord) == 512);

    static constexpr sm::uuid kEfiSystemPartition = sm::uuid::of("C12A7328-F81F-11D2-BA4B-00A0C93EC93B");
    static constexpr sm::uuid kWindowsNtRecovery = sm::uuid::of("DE94BBA4-06D1-4D40-A16A-BFD50179D6AC");

    struct [[gnu::packed]] GptHeader {
        static constexpr std::array<char, 8> kSignature = util::Signature("EFI PART");

        std::array<char, 8> signature;
        sm::le<uint32_t> revision;
        sm::le<uint32_t> size;
        sm::le<uint32_t> checksum;
        uint8_t reserved[4];
        sm::le<uint64_t> lbaCurrent;
        sm::le<uint64_t> lbaBackup;
        sm::le<uint64_t> lbaPartitionBegin;
        sm::le<uint64_t> lbaParitionEnd;
        sm::uuid guid;
        sm::le<uint64_t> lbaPartitionArray;
        sm::le<uint32_t> partitionCount;
        sm::le<uint32_t> partitionEntrySize;
        sm::le<uint32_t> partitionArrayChecksum;
    };

    static_assert(sizeof(GptHeader) == 92);

    struct GptEntry {
        static constexpr sm::uuid kNullEntry = sm::uuid::nil();
        sm::uuid type;
        sm::uuid guid;
        sm::le<uint64_t> lbaBegin;
        sm::le<uint64_t> lbaEnd;
        uint64_t flags;
        std::array<char16_t, 36> name;
    };

    static_assert(sizeof(GptEntry) == 128);

    struct DrivePartitions {
        MasterBootRecord mbr;
        GptHeader gpt;
        stdx::Vector2<GptEntry> entries;

        std::span<const GptEntry> gptPartitions() const {
            return std::span(entries);
        }
    };

    enum class PartParseError {
        eReadError,
        eInvalidMbrSignature,
        eInvalidGptSignature,
    };

    std::expected<DrivePartitions, PartParseError> ParsePartitionTable(BlockDevice& drive);
}
