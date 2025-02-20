#pragma once

#include <bezos/key.h>
#include <bezos/subsystem/hid.h>

#include "hid/ps2.hpp"
#include "notify.hpp"
#include "isr.hpp"

namespace hid {
    static constexpr sm::uuid kHidEvent = sm::uuid::of("dd4ece3c-ec81-11ef-8b71-c761047b867e");

    class HidNotification : public km::INotification {
        OsHidEvent mEvent;

    public:
        HidNotification(OsHidEvent event)
            : INotification(km::SendTime{})
            , mEvent(event)
        { }

        OsHidEvent event() const {
            return mEvent;
        }
    };

    void InitPs2HidStream(km::NotificationStream *stream);
    km::Topic *GetHidPs2Topic();

    const km::IsrEntry *InstallPs2KeyboardIsr(km::IoApicSet& ioApicSet, hid::Ps2Controller& controller, const km::IApic *target, km::IsrTable *ist);
    const km::IsrEntry *InstallPs2MouseIsr(km::IoApicSet& ioApicSet, hid::Ps2Controller& controller, const km::IApic *target, km::IsrTable *ist);
}
