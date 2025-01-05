#pragma once

namespace stdx {
    class ConditionVariable {
    public:
        void wait();
        void notifyOne();
        void notifyAll();
    };
}
