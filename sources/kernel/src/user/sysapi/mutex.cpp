#include "process/schedule.hpp"
#include "user/sysapi.hpp"

#include "process/mutex.hpp"
#include "process/system.hpp"

#include "syscall.hpp"

#include <bezos/facility/mutex.h>

// TODO: make this runtime configurable
static constexpr size_t kMaxPathSize = 0x1000;

OsCallResult um::MutexCreate(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userCreateInfo = regs->arg0;

    OsMutexCreateInfo createInfo{};
    stdx::String name;

    if (OsStatus status = context->readObject(userCreateInfo, &createInfo)) {
        return km::CallError(status);
    }

    if (OsStatus status = context->readString((uint64_t)createInfo.NameFront, (uint64_t)createInfo.NameBack, kMaxPathSize, &name)) {
        return km::CallError(status);
    }

    km::Mutex *mutex = system->objects->createMutex(std::move(name));
    return km::CallOk(mutex->publicId());
}

OsCallResult um::MutexDestroy(km::System *, km::CallContext *, km::SystemCallRegisterSet *) {
    return km::CallError(OsStatusNotSupported);
}

// Clang can't analyze the control flow here because its all very non-local.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wthread-safety-analysis"

OsCallResult um::MutexLock(km::System *system, km::CallContext *, km::SystemCallRegisterSet *regs) {
    uint64_t id = regs->arg0;
    km::Mutex *mutex = system->objects->getMutex(km::MutexId(OS_HANDLE_ID(id)));

    if (mutex == nullptr) {
        return km::CallError(OsStatusNotFound);
    }

    //
    // Try a few times to acquire the lock before putting the thread to sleep.
    //
    for (int i = 0; i < 10; i++) {
        if (mutex->lock.try_lock()) {
            return km::CallOk(0zu);
        }
    }

    while (!mutex->lock.try_lock()) {
        km::YieldCurrentThread();
    }

    return km::CallOk(0zu);
}

OsCallResult um::MutexUnlock(km::System *system, km::CallContext *, km::SystemCallRegisterSet *regs) {
    uint64_t id = regs->arg0;
    km::Mutex *mutex = system->objects->getMutex(km::MutexId(OS_HANDLE_ID(id)));
    if (mutex == nullptr) {
        return km::CallError(OsStatusNotFound);
    }

    mutex->lock.unlock();
    return km::CallOk(0zu);
}

#pragma clang diagnostic pop
