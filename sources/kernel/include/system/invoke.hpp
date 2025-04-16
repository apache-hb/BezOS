#pragma once

#include "system/access.hpp" // IWYU pragma: export

#include "std/rcuptr.hpp"

namespace sys2 {
    class InvokeContext {
    public:
        /// @brief The system context.
        System *system;

        /// @brief The process namespace this method is being invoked in.
        sm::RcuSharedPtr<Process> process;

        /// @brief The thread in the process that is invoking the method.
        sm::RcuSharedPtr<Thread> thread;

        /// @brief The transaction context that this method is contained in.
        OsTxHandle tx;

    public:
        InvokeContext(System *system, sm::RcuSharedPtr<Process> process, sm::RcuSharedPtr<Thread> thread = nullptr, OsTxHandle tx = OS_HANDLE_INVALID)
            : system(system)
            , process(process)
            , thread(thread)
            , tx(tx)
        { }
    };
}
