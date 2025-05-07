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
        OsStatus SendEvent(const EventPacket &packet) noexcept [[clang::nonallocating]];
    }

    template<typename T>
    consteval Event EventType() = delete;

    template<>
    consteval Event EventType<AllocatePhysicalMemory>() {
        return Event::eAllocatePhysicalMemory;
    }

    template<>
    consteval Event EventType<AllocateVirtualMemory>() {
        return Event::eAllocateVirtualMemory;
    }

    template<>
    consteval Event EventType<ReleasePhysicalMemory>() {
        return Event::eReleasePhysicalMemory;
    }

    template<>
    consteval Event EventType<ReleaseVirtualMemory>() {
        return Event::eReleaseVirtualMemory;
    }

    template<>
    consteval Event EventType<ScheduleTask>() {
        return Event::eScheduleTask;
    }

    OsStatus InitDebugStream(ComPortInfo info);

    template<typename T>
    OsStatus SendEvent(const T& event) noexcept [[clang::nonallocating]] {
        if constexpr (KM_DEBUG_EVENTS) {
            //
            // Subobject initialization weirdness, can't be bothered to fix it.
            //
            EventPacket packet {
                .event = EventType<T>(),
            };
            memcpy(&packet.data, &event, sizeof(T));

            return detail::SendEvent(packet);
        } else {
            return OsStatusSuccess;
        }
    }
}
