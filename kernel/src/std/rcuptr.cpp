#include "std/rcuptr.hpp"

namespace rcu = sm::rcu::detail;

void rcu::RcuReleaseStrong(void *ptr) {
    ControlBlock *cb = static_cast<ControlBlock*>(ptr);
    if (cb->strong.decrement()) {
        cb->deleter(cb->value);
    }

    RcuReleaseWeak(cb);
}

bool rcu::RcuAcqiureStrong(ControlBlock& control) {
    bool result = RcuAcquireWeak(control) && control.strong.increment();
    if (!result) {
        RcuReleaseWeak(&control);
    }

    return result;
}

void rcu::RcuReleaseWeak(void *ptr) {
    ControlBlock *cb = static_cast<ControlBlock*>(ptr);
    if (cb->weak.decrement()) {
        delete cb;
    }
}

bool rcu::RcuAcquireWeak(ControlBlock& control) {
    bool result = control.weak.increment();
    return result;
}
