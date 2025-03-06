#pragma once

#include <stdint.h>

namespace km::debug {
    enum class Event : uint8_t {
        eAck = 0,
        eAllocatePhysicalMemory = 1,
        eAllocateVirtualMemory = 2,
        eReleasePhysicalMemory = 3,
        eReleaseVirtualMemory = 4,
    };

    struct AllocatePhysicalMemory {
        uint64_t size;
        uint64_t address;
        uint32_t alignment;
        uint32_t tag;
    };

    struct AllocateVirtualMemory {
        uint64_t size;
        uint64_t address;
        uint32_t alignment;
        uint32_t tag;
    };

    struct ReleasePhysicalMemory {
        uint64_t begin;
        uint64_t end;
        uint32_t tag;
    };

    struct ReleaseVirtualMemory {
        uint64_t begin;
        uint64_t end;
        uint32_t tag;
    };

    struct EventPacket {
        Event event;
        union {
            AllocatePhysicalMemory allocatePhysicalMemory;
            AllocateVirtualMemory allocateVirtualMemory;
            ReleasePhysicalMemory releasePhysicalMemory;
            ReleaseVirtualMemory releaseVirtualMemory;
        } data;
    };
}
