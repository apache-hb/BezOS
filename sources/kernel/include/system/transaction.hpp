#pragma once

#include <bezos/facility/tx.h>

#include "system/handle.hpp"

namespace sys2 {
    class System;
    class Tx;
    class TxHandle;

    enum class TxAccess : OsHandleAccess {
        eNone = eOsTxAccessNone,
        eWait = eOsTxAccessWait,
        eCommit = eOsTxAccessCommit,
        eRollback = eOsTxAccessRollback,
        eStat = eOsTxAccessStat,
        eWrite = eOsTxAccessWrite,
        eAll = eOsTxAccessAll,
    };

    class TxHandle final : public IHandle {
        sm::RcuWeakPtr<Tx> mTransaction;
        OsHandle mHandle;
        TxAccess mAccess;

    public:
        sm::RcuWeakPtr<IObject> getObject() override;
        OsHandle getHandle() const override { return mHandle; }
    };

    class Tx final : public IObject {
        stdx::SharedSpinLock mLock;
        ObjectName mName GUARDED_BY(mLock);

    public:
        void setName(ObjectName name) override;
        ObjectName getName() override;

        stdx::StringView getClassName() const override { return "Transaction"; }
    };
}
