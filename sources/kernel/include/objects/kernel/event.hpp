#pragma once

#include "objects/generic/event.hpp"

namespace km {
    class KernelEvent : public GenericEvent {
        OsEventHandle mEvent;
    public:
        KernelEvent();
        ~KernelEvent();

        OsStatus signal() {
            return sys2::SysEventSignal(mEvent);
        }

        OsStatus wait(OsInstant timeout) {
            return sys2::SysHandleWait(mEvent, timeout);
        }
    };

    using Event = KernelEvent;
}
