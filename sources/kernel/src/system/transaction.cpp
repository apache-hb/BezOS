#include "system/transaction.hpp"

sys::TxHandle::TxHandle(sm::RcuSharedPtr<Tx> transaction, OsHandle handle, TxAccess access)
    : BaseHandle(transaction, handle, access)
{ }

sys::Tx::Tx(ObjectName name)
    : BaseObject(name)
{ }
