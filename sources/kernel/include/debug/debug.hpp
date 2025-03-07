#pragma once

#include <bezos/status.h>

#include "debug/packet.hpp"

#include "uart.hpp"

#if UTIL_TESTING
#   define KM_DEBUG_EVENTS 0
#else
#   define KM_DEBUG_EVENTS 1
#endif

namespace km::debug {
    namespace detail {
        OsStatus SendEvent(const EventPacket &packet);
    }

    template<typename T>
    constexpr Event EventType() = delete;

    template<>
    constexpr Event EventType<AllocatePhysicalMemory>() {
        return Event::eAllocatePhysicalMemory;
    }

    template<>
    constexpr Event EventType<AllocateVirtualMemory>() {
        return Event::eAllocateVirtualMemory;
    }

    template<>
    constexpr Event EventType<ReleasePhysicalMemory>() {
        return Event::eReleasePhysicalMemory;
    }

    template<>
    constexpr Event EventType<ReleaseVirtualMemory>() {
        return Event::eReleaseVirtualMemory;
    }

    template<>
    constexpr Event EventType<ScheduleTask>() {
        return Event::eScheduleTask;
    }

    OsStatus InitDebugStream(ComPortInfo info);

    template<typename T>
    OsStatus SendEvent([[maybe_unused]] const T& event) {
#if KM_DEBUG_EVENTS
        //
        // Subobject initialization weirdness, can't be bothered to fix it.
        //
        EventPacket packet {
            .event = EventType<T>(),
        };
        memcpy(&packet.data, &event, sizeof(T));

        return detail::SendEvent(packet);
#else
        return OsStatusSuccess;
#endif
    }
}
