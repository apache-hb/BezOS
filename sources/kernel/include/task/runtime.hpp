#pragma once

namespace km {
    struct IsrContext;
}

namespace task {
    class Scheduler;
    class SchedulerQueue;

    /// @brief Try to switch to a new task.
    ///
    /// This swaps out the current xsave state and tls address, but does not modify the current syscall stack
    /// the syscall stack must be swapped out by the caller to the new tasks stack.
    ///
    /// @param scheduler The scheduler to use for the context switch.
    /// @param queue This cores scheduler queue.
    /// @param isrContext The ISR context to use for the context switch.
    /// @return If a new task has been selected, this implies the invoker must switch extra state to setup the new task environment.
    bool switchCurrentContext(Scheduler *scheduler, task::SchedulerQueue *queue, km::IsrContext *isrContext) noexcept;
}
