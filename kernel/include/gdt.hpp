#pragma once

#include "util/util.hpp"

#include <stdint.h>

namespace km {
    enum class DescriptorFlags {
        eLong = (1 << 1),
        eSize = (1 << 2),
        eGranularity = (1 << 3),
    };

    UTIL_BITFLAGS(DescriptorFlags);

    enum class SegmentAccessFlags {
        eAccessed = (1 << 0),
        eReadWrite = (1 << 1),
        eEscalateDirection = (1 << 2),
        eExecutable = (1 << 3),
        eCodeOrDataSegment = (1 << 4),

        eRing0 = (0 << 5),
        eRing1 = (1 << 5),
        eRing2 = (2 << 5),
        eRing3 = (3 << 5),

        ePresent = (1 << 7),
    };

    UTIL_BITFLAGS(SegmentAccessFlags);

    constexpr uint64_t KmBuildSegmentDescriptor(DescriptorFlags flags, SegmentAccessFlags access, uint16_t limit) {
        return (((uint64_t)(flags) << 52) | ((uint64_t)(access) << 40)) | (0xFULL << 48) | (uint16_t)limit;
    }
}

void KmInitGdt(const uint64_t *gdt, uint64_t count, uint64_t codeSelector, uint64_t dataSelector);
