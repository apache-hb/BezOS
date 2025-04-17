#include "user/sysapi.hpp"

#include "syscall.hpp"

#include <bezos/facility/mutex.h>

// Clang can't analyze the control flow here because its all very non-local.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wthread-safety-analysis"

OsCallResult um::MutexLock(km::System *, km::CallContext *, km::SystemCallRegisterSet *) {
    return km::CallError(OsStatusNotSupported);
}

OsCallResult um::MutexUnlock(km::System *, km::CallContext *, km::SystemCallRegisterSet *) {
    return km::CallError(OsStatusNotSupported);
}

#pragma clang diagnostic pop
