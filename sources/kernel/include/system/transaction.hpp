#pragma once

#include "system/base.hpp"
#include "system/create.hpp"

namespace sys {
    class Tx final : public BaseObject<eOsHandleTx> {
    public:
        using Access = TxAccess;

        Tx(ObjectName name);

        stdx::StringView getClassName() const override { return "Transaction"; }
    };

    class TxHandle final : public BaseHandle<Tx> {
    public:
        TxHandle(sm::RcuSharedPtr<Tx> transaction, OsHandle handle, TxAccess access);

        sm::RcuSharedPtr<Tx> getTransaction() const { return getInner(); }

        OsStatus createProcess(System *system, ProcessCreateInfo info, ProcessHandle **handle);
        OsStatus createThread(System *system, ProcessHandle *process, ThreadCreateInfo info, ThreadHandle **handle);
    };
}
