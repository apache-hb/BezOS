#include "system/transaction.hpp"

sys2::TxHandle::TxHandle(sm::RcuSharedPtr<Tx> transaction, OsHandle handle, TxAccess access)
    : Super(transaction, handle, access)
{ }

sys2::Tx::Tx(const TxCreateInfo& createInfo)
    : Super(createInfo.name)
{ }
