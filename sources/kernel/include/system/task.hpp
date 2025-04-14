#pragma once

#include <bezos/status.h>

#include <stdint.h>

namespace sys2 {
    struct AffinityMask {
        uint64_t mask[4] = { [0 ... 3] = UINT64_MAX }; // TODO: support more than 256 processors
    };

    struct TaskScheduleState {
        AffinityMask affinity;
    };

    class ITask {
    public:
        virtual ~ITask() = default;

        /// @brief Resume the task from a suspended state
        [[noreturn]]
        virtual void resume() = 0;

        /// @brief Save the current state of the task to be resumed later
        virtual void suspend() = 0;

        virtual OsStatus getScheduleState(TaskScheduleState *info) = 0;
    };
}
