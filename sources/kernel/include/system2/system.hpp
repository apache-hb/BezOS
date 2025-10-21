#pragma once

#include "system2/pmm.hpp"
#include "system2/vmm.hpp"
#include "task/scheduler.hpp"

#include <bezos/bezos.h>

namespace obj {
    class System {
        task::Scheduler mScheduler;

        sm::RcuDomain mDomain;
    public:
        sm::RcuDomain& getDomain() noexcept {
            return mDomain;
        }

        [[gnu::returns_nonnull]]
        obj::IPhysicalMemoryService *getPhysicalMemoryService() noexcept;

        [[gnu::returns_nonnull]]
        obj::IVirtualMemoryService *getVirtualMemoryService() noexcept;

        [[nodiscard]]
        static OsStatus create(System *result [[outparam]]) noexcept;
    };
}
