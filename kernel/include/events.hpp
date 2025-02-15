#pragma once

#include "std/shared.hpp"
#include <concurrentqueue.h>

namespace km {
    class IEvent : public sm::IntrusiveCount<IEvent> {

    };

    class EventBuffer {
        moodycamel::ConcurrentQueue<sm::SharedPtr<IEvent>> mQueue;
    };
}
