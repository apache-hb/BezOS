#include "std/rcuptr.hpp"

namespace rcu = sm::rcu::detail;

void rcu::RcuReleaseStrong(ControlBlock *control) noexcept [[clang::reentrant, clang::nonblocking]] {
    //
    // If the final strong reference is released then delete the object data.
    // The control block is retained until the weak reference count reaches zero.
    //
    auto flags = control->count.strongRelease();
    if (flags == JointCount::eNone) {
        return;
    }

    sm::RcuGuard guard(*control->domain);
    if (bool(flags & JointCount::eStrong)) {
        control->token->value = control->value;
        guard.enqueue(control->token, control->deleter);
    }

    if (bool(flags & JointCount::eWeak)) {
        guard.retire(control);
    }
}

bool rcu::RcuAcqiureStrong(ControlBlock *control) noexcept [[clang::reentrant, clang::nonblocking]] {
    return control->count.strongRetain();
}

void rcu::RcuReleaseWeak(ControlBlock *control) noexcept [[clang::reentrant, clang::nonblocking]] {
    if (control->count.weakRelease()) {
        control->domain->retire(control);
    }
}

bool rcu::RcuAcquireWeak(ControlBlock *control) noexcept [[clang::reentrant, clang::nonblocking]] {
    return control->count.weakRetain();
}
