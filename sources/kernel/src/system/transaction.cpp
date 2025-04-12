#include "system/transaction.hpp"

sys2::TxHandle::TxHandle(sm::RcuSharedPtr<Tx> transaction, OsHandle handle, TxAccess access)
    : BaseHandle(transaction, handle, access)
{ }

sys2::Tx::Tx(ObjectName name)
    : BaseObject(name)
{ }
