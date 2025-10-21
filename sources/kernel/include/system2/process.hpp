#pragma once

#include "memory/page_tables.hpp"
#include "system2/object.hpp"
#include "system2/pmm.hpp"
#include "system2/vmm.hpp"

namespace obj {
    class ProcessCreateInfo {
        ObjectName mName;
        sm::RcuSharedPtr<Process> mParentProcess;

    public:
        const ObjectName& getName() const { return mName; }
        sm::RcuSharedPtr<Process> getParentProcess() const { return mParentProcess; }

        [[nodiscard]]
        static OsStatus create(ObjectName name, sm::RcuSharedPtr<Process> parent, ProcessCreateInfo *info [[outparam]]) noexcept {
            if (!BaseObject::verifyObjectName(name)) {
                return OsStatusInvalidInput;
            }

            info->mName = name;
            info->mParentProcess = parent;
            return OsStatusSuccess;
        }
    };

    class Process : public BaseObject {
        sm::RcuWeakPtr<Process> mParentProcess;

        std::unique_ptr<IPhysicalMemoryClient> mPhysicalMemoryClient;
        std::unique_ptr<IVirtualMemoryClient> mVirtualMemoryClient;
        km::PageTables mPageTables;

        System *mSystem;
    public:
        OsStatus destroy(uint32_t reason, int64_t exitCode) noexcept;

        [[nodiscard]]
        static OsStatus create(System *system, const ProcessCreateInfo& createInfo, sm::RcuSharedPtr<Process> *result [[outparam]]) noexcept [[clang::allocating]];
    };
}
