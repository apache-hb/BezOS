#pragma once

#include "hid/ps2.hpp"
#include "notify.hpp"
#include "isr.hpp"

namespace hid {
    static constexpr sm::uuid kHidEvent = sm::uuid::of("dd4ece3c-ec81-11ef-8b71-c761047b867e");

    enum class HidEventType {
        eKeyDown,
        eKeyUp,
        eMouseMove,
    };

    struct HidMouseEvent {
        int32_t dx;
        int32_t dy;
    };

    struct HidKeyEvent {
        uint32_t code;
    };

    struct HidEvent {
        HidEventType type;
        union {
            HidMouseEvent move;
            HidKeyEvent key;
        };
    };

    class HidNotification : public km::INotification {
        HidEvent mEvent;

    public:
        HidNotification(HidEvent event);

        HidEvent event() const {
            return mEvent;
        }
    };

    const km::IsrTable::Entry *InstallPs2KeyboardIsr(km::IoApicSet& ioApicSet, hid::Ps2Controller& controller, const km::IApic *target, km::IsrTable *ist, km::NotificationStream *stream);
    const km::IsrTable::Entry *InstallPs2MouseIsr(km::IoApicSet& ioApicSet, hid::Ps2Controller& controller, const km::IApic *target, km::IsrTable *ist, km::NotificationStream *stream);
}
