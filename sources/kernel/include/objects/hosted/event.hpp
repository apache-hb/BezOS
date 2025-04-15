#pragma once

#include "objects/generic/event.hpp"
#include <condition_variable>
#include <mutex>

namespace km {
    class HostedEvent : public GenericEvent {
        std::mutex mMutex;
        std::condition_variable mCondition;
    public:
        HostedEvent();
        ~HostedEvent();

        OsStatus signal() {
            mCondition.notify_all();
            return OsStatusSuccess;
        }

        OsStatus wait(OsInstant timeout) {
            std::unique_lock<std::mutex> lock(mMutex);
            if (timeout == OS_TIMEOUT_INFINITE) {
                mCondition.wait(lock);
            } else {
                if (mCondition.wait_for(lock, std::chrono::nanoseconds(timeout * 100)) == std::cv_status::timeout) {
                    return OsStatusTimeout;
                }
            }
            return OsStatusSuccess;
        }
    };

    using Event = HostedEvent;
}
