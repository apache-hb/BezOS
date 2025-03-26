#include "std/rcuptr.hpp"

namespace rcu = sm::rcu::detail;

void rcu::RcuReleaseStrong(ControlBlock *cb) {
    //
    // If the final strong reference is released then delete the object data.
    // The control block is retained until the weak reference count reaches zero.
    //
    auto flags = cb->count.strongRelease();
    if (bool(flags & JointCount::eStrong)) {
        cb->domain->call(cb, cb->deleter);
    }

    if (bool(flags & JointCount::eWeak)) {
        cb->domain->retire(cb);
    }
}

bool rcu::RcuAcqiureStrong(ControlBlock *control) {
    return control->count.strongRetain();
}

void rcu::RcuReleaseWeak(ControlBlock *cb) {
    if (cb->count.weakRelease()) {
        cb->domain->retire(cb);
    }
}

bool rcu::RcuAcquireWeak(ControlBlock *control) {
    return control->count.weakRetain();
}
