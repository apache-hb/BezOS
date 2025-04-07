#include "system/transaction.hpp"

sys2::TxHandle::TxHandle(sm::RcuSharedPtr<Tx> transaction, OsHandle handle, TxAccess access)
    : mTransaction(transaction)
    , mHandle(handle)
    , mAccess(access)
{ }

sm::RcuWeakPtr<sys2::IObject> sys2::TxHandle::getObject() {
    return mTransaction;
}

sys2::Tx::Tx(const TxCreateInfo& createInfo)
    : mName(createInfo.name)
{ }

void sys2::Tx::setName(ObjectName name) {
    stdx::UniqueLock guard(mLock);
    mName = name;
}

sys2::ObjectName sys2::Tx::getName() {
    stdx::SharedLock guard(mLock);
    return mName;
}

