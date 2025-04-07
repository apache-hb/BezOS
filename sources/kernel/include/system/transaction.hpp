#pragma once

#include "system/handle.hpp"
#include "system/create.hpp"

namespace sys2 {
    class TxHandle final : public IHandle {
        sm::RcuSharedPtr<Tx> mTransaction;
        OsHandle mHandle;
        TxAccess mAccess;

    public:
        TxHandle(sm::RcuSharedPtr<Tx> transaction, OsHandle handle, TxAccess access);

        sm::RcuSharedPtr<Tx> getTransaction() const { return mTransaction; }

        sm::RcuWeakPtr<IObject> getObject() override;
        OsHandle getHandle() const override { return mHandle; }

        bool hasAccess(TxAccess access) const {
            return bool(mAccess & access);
        }

        OsStatus createProcess(System *system, ProcessCreateInfo info, ProcessHandle **handle);
        OsStatus createThread(System *system, ProcessHandle *process, ThreadCreateInfo info, ThreadHandle **handle);
    };

    class Tx final : public IObject {
        stdx::SharedSpinLock mLock;
        ObjectName mName GUARDED_BY(mLock);

    public:
        Tx(const TxCreateInfo& createInfo);

        void setName(ObjectName name) override;
        ObjectName getName() override;

        stdx::StringView getClassName() const override { return "Transaction"; }
    };
}
