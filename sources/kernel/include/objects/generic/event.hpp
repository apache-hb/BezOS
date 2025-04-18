#pragma once

#include <bezos/handle.h>

#include "common/util/util.hpp"

namespace km {
    class GenericEvent {
    public:
        UTIL_NOCOPY(GenericEvent);
        UTIL_NOMOVE(GenericEvent);

        [[gnu::error("Event::signal() not implemented by platform")]]
        OsStatus signal();

        [[gnu::error("Event::wait(OsInstant) not implemented by platform")]]
        OsStatus wait(OsInstant timeout);
    };
}
