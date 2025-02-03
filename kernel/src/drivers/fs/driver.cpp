#include "drivers/fs/driver.hpp"

std::expected<km::DrivePartitions, km::PartParseError> km::ParsePartitionTable(DriveMedia& drive) {
    MasterBootRecord mbr;

    if (drive.read(0, &mbr, sizeof(MasterBootRecord)) != sizeof(MasterBootRecord))
        return std::unexpected(km::PartParseError::eReadError);

    if (mbr.signature != MasterBootRecord::kSignature)
        return std::unexpected(km::PartParseError::eInvalidMbrSignature);

    if (!mbr.isGpt()) {
        return DrivePartitions {
            .mbr = mbr,
        };
    }

    GptHeader header;
    if (drive.read(drive.blockSize(), &header, sizeof(GptHeader)) != sizeof(GptHeader))
        return std::unexpected(km::PartParseError::eReadError);

    if (header.signature != GptHeader::kSignature)
        return std::unexpected(km::PartParseError::eInvalidMbrSignature);

    std::unique_ptr<GptEntry[]> entries(new GptEntry[header.partitionCount]);

    for (uint32_t i = 0; i < header.partitionCount; i++) {
        GptEntry entry;
        if (drive.read(header.lbaPartitionArray * drive.blockSize() + i * header.partitionEntrySize, &entry, sizeof(GptEntry)) != sizeof(GptEntry))
            continue;

        entries[i] = entry;
    }

    return DrivePartitions {
        .mbr = mbr,
        .gpt = header,
        .entries = std::move(entries),
    };
}
