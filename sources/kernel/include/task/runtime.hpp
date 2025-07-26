#pragma once

namespace km {
    struct IsrContext;
}

namespace task {
    class Scheduler;
    class SchedulerQueue;

    /// @brief Try to switch to a new task.
    ///
    /// @param scheduler The scheduler to use for the context switch.
    /// @param queue This cores scheduler queue.
    /// @param isrContext The ISR context to use for the context switch.
    /// @return If a new task is ready
    void switchCurrentContext(Scheduler *scheduler, task::SchedulerQueue *queue, km::IsrContext *isrContext) noexcept;
}
