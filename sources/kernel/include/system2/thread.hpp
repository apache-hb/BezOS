#pragma once

#include "system2/object.hpp"

namespace obj {
    class ThreadCreateInfo {
        ObjectName mName;
        sm::RcuSharedPtr<Process> mParentProcess;

    public:
        const ObjectName& getName() const { return mName; }
        sm::RcuSharedPtr<Process> getParentProcess() const { return mParentProcess; }

        [[nodiscard]]
        static OsStatus create(ObjectName name, sm::RcuSharedPtr<Process> parent, ThreadCreateInfo *info [[outparam]]) noexcept {
            if (!BaseObject::verifyObjectName(name)) {
                return OsStatusInvalidInput;
            }

            info->mName = name;
            info->mParentProcess = parent;
            return OsStatusSuccess;
        }
    };

    class Thread : public BaseObject {
        sm::RcuWeakPtr<Process> mParentProcess;

        System *mSystem;
    public:
        [[nodiscard]]
        static OsStatus create(System *system, const ThreadCreateInfo& createInfo, sm::RcuSharedPtr<Thread> *result [[outparam]]) noexcept [[clang::allocating]];
    };
}
