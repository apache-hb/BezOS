#pragma once

#include "objects/generic/event.hpp"

namespace km {
    class KernelEvent : public GenericEvent {
        OsEventHandle mEvent;
    public:
        KernelEvent();
        ~KernelEvent();

        OsStatus signal() {
            return sys::SysEventSignal(mEvent);
        }

        OsStatus wait(OsInstant timeout) {
            return sys::SysHandleWait(mEvent, timeout);
        }
    };

    using Event = KernelEvent;
}
