#include "std/rcuptr.hpp"

namespace rcu = sm::rcu::detail;

void rcu::RcuReleaseStrong(ControlBlock *cb) {
    //
    // If the final strong reference is released then delete the object data.
    // The control block is retained until the weak reference count reaches zero.
    //
    if (cb->strong.decrement()) {
        cb->domain->call(cb->value, cb->deleter);
    }

    RcuReleaseWeak(cb);
}

bool rcu::RcuAcqiureStrong(ControlBlock *control) {
    bool result = RcuAcquireWeak(control) && control->strong.increment();
    if (!result) {
        RcuReleaseWeak(control);
    }

    return result;
}

void rcu::RcuReleaseWeak(ControlBlock *cb) {
    if (cb->weak.decrement()) {
        cb->domain->retire(cb);
    }
}

bool rcu::RcuAcquireWeak(ControlBlock *control) {
    bool result = control->weak.increment();
    return result;
}
