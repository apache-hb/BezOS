#pragma once

#include "drivers/fs/driver.hpp"

namespace km {
    struct [[gnu::packed]] FatBootSector {
        std::byte jmp[3];
        std::array<char, 8> oemName;
        sm::le<uint16_t> bytesPerSector;
        uint8_t sectorsPerCluster;
        sm::le<uint16_t> reservedSectors;
        uint8_t fatCount;
        sm::le<uint16_t> rootEntryCount;
        sm::le<uint16_t> totalSectors16;
        uint8_t mediaType;
        sm::le<uint16_t> sectorsPerFat16;
        sm::le<uint16_t> sectorsPerTrack;
        sm::le<uint16_t> headCount;
        sm::le<uint32_t> hiddenSectors;
        sm::le<uint32_t> totalSectors32;
        sm::le<uint32_t> sectorsPerFat32;
        uint16_t flags;
        sm::le<uint16_t> version;
        sm::le<uint32_t> rootCluster;
        sm::le<uint16_t> fsInfoSector;
        sm::le<uint16_t> backupBootSector;
        std::byte reserved0[12];
        uint8_t driveNumber;
        std::byte reserved1[1];
        uint8_t signature;
        sm::le<uint32_t> volumeId;
        std::array<char, 11> volumeLabel;
        std::array<char, 8> fsType;
        std::array<char, 420> bootCode;
        std::array<uint8_t, 2> bootSignature;

        uint32_t rootSectorCount() const {
            return ((rootEntryCount * 32) + (bytesPerSector - 1)) / bytesPerSector;
        }

        uint64_t dataSectorCount() const {
            uint64_t sz = (sectorsPerFat16 != 0) ? sectorsPerFat16 : sectorsPerFat32;
            uint64_t sectors = (totalSectors16 != 0) ? totalSectors16 : totalSectors32;

            return sectors - (reservedSectors + (fatCount * sz) + rootSectorCount());
        }

        uint64_t totalClusters() const {
            return dataSectorCount() / sectorsPerCluster;
        }
    };

    static_assert(sizeof(FatBootSector) == 512);

    struct FatInfo {
        static constexpr std::array<uint8_t, 4> kLeadSignature = { 0x41, 0x61, 0x52, 0x52 };
        static constexpr std::array<uint8_t, 4> kStructureSignature = { 0x61, 0x41, 0x72, 0x72 };
        static constexpr std::array<uint8_t, 4> kTrailSignature = { 0xAA, 0x55, 0x00, 0x00 };

        std::array<uint8_t, 4> leadSignature;
        std::byte reserved0[480];
        std::array<uint8_t, 4> structureSignature;
        sm::le<uint32_t> freeClusterCount;
        sm::le<uint32_t> nextFreeCluster;
        std::byte reserved1[12];
        std::array<uint8_t, 4> trailSignature;
    };

    static_assert(sizeof(FatInfo) == 512);

    struct FatFolder {
        char name[11];
        uint8_t attributes;
        std::byte reserved0[1];
        uint8_t creationTimeTenths;
        sm::le<uint16_t> creationTime;
        sm::le<uint16_t> creationDate;
        sm::le<uint16_t> lastAccessDate;
        sm::le<uint16_t> firstClusterHigh;
        sm::le<uint16_t> lastWriteTime;
        sm::le<uint16_t> lastWriteDate;
        sm::le<uint16_t> firstClusterLow;
        sm::le<uint32_t> fileSize;
    };

    static_assert(sizeof(FatFolder) == 32);

    class FatFsDriver : public km::IFsDriver {

    };
}
