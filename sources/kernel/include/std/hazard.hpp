#pragma once

#include <atomic>

namespace sm::hazard {
    class HazardObject {

    };

    class RetireList {
        HazardObject *mHead;
        HazardObject *mTail;

    public:
        RetireList();
    };

    class HazardSlot {
        std::atomic<HazardObject*> mObject;
    };

    class HazardBoard {

    };
}
