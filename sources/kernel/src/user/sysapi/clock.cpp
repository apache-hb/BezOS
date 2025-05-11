#include "user/context.hpp"
#include "user/sysapi.hpp"

#include "syscall.hpp"

#include "clock.hpp"

#include <bezos/facility/clock.h>

constexpr km::VerifyRules kClockStatRules {
    .alignment = alignof(OsClockInfo),
    .minSize = sizeof(OsClockInfo),
    .maxSize = sizeof(OsClockInfo),
};

OsCallResult um::ClockStat(km::System *system, km::CallContext *context, km::SystemCallRegisterSet *regs) {
    uint64_t userClockInfo = regs->arg0;
    OsClockInfo info{};

    if (OsStatus status = system->clock->stat(&info)) {
        return km::CallError(status);
    }

    km::SystemMemory *memory = system->memory;
    km::PageBuilder *pm = &memory->getPageManager();
    km::PageTables *pt = &memory->systemTables();

    if (OsStatus status = km::WriteUserObject(kClockStatRules, userClockInfo, sizeof(OsClockInfo), &info, pm, pt)) {
        return km::CallError(status);
    }

    if (OsStatus status = context->writeObject(userClockInfo, info)) {
        return km::CallError(status);
    }

    return km::CallOk(0zu);
}

OsCallResult um::ClockTime(km::System *system, km::CallContext *, km::SystemCallRegisterSet *) {
    OsInstant time{};
    if (OsStatus status = system->clock->time(&time)) {
        return km::CallError(status);
    }

    return km::CallOk(time);
}

OsCallResult um::ClockTicks(km::System *system, km::CallContext *, km::SystemCallRegisterSet *) {
    OsTickCounter ticks{};
    if (OsStatus status = system->clock->ticks(&ticks)) {
        return km::CallError(status);
    }

    return km::CallOk(ticks);
}
