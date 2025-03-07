#pragma once

#include <bezos/status.h>

#include "debug/packet.hpp"

#include "uart.hpp"

namespace km::debug {
    static constexpr bool kEnableDebugEvents = true;

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
    OsStatus SendEvent(const EventPacket &packet);

    template<typename T>
    OsStatus SendEvent(const T& event) {
        if constexpr (kEnableDebugEvents) {
            //
            // Subobject initialization weirdness, can't be bothered to fix it.
            //
            EventPacket packet {
                .event = EventType<T>(),
            };
            memcpy(&packet.data, &event, sizeof(T));

            return SendEvent(packet);
        }

        return OsStatusSuccess;
    }
}
