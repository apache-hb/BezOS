#pragma once

#include "system/base.hpp"

namespace sys2 {
    class Event : public BaseObject<eOsHandleEvent> {
    public:
        using Access = EventAccess;
    };

    class EventHandle : public BaseHandle<Event, eOsHandleEvent> {

    };
}
