#pragma once

#include <bezos/status.h>

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
        uint64_t begin;
        uint64_t end;
    };

    struct AllocateVirtualMemory {
        uint64_t begin;
        uint64_t end;
    };

    struct ReleasePhysicalMemory {
        uint64_t begin;
        uint64_t end;
    };

    struct ReleaseVirtualMemory {
        uint64_t begin;
        uint64_t end;
    };

    struct EventPacket {
        Event event;
        union {
            AllocatePhysicalMemory allocatePhysicalMemory;
            AllocateVirtualMemory allocateVirtualMemory;
            ReleasePhysicalMemory releasePhysicalMemory;
            ReleaseVirtualMemory releaseVirtualMemory;
        };
    };

    OsStatus SendEvent(const EventPacket &packet);
}
