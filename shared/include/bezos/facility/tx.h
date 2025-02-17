#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @defgroup OsTransaction Transactions
/// @{

#define OS_TRANSACTION_INVALID ((OsTransactionHandle)(0))

/// @brief Transaction creation modes.
///
/// Bits [0:1] contain the read transaction mode.
/// Bits [2:3] contain the write transaction mode.
/// Bits [4:63] are reserved and must be zero.
typedef uint64_t OsTransactionMode;

enum {
    eOsIsolateReadSerializable = UINT64_C(0),
    eOsIsolateReadCommitted    = UINT64_C(1),
    eOsIsolateReadUncommitted  = UINT64_C(2),

    eOsIsolateWriteSerializable = UINT64_C(0) << 2,
    eOsIsolateWriteCommitted    = UINT64_C(1) << 2,
    eOsIsolateWriteUncommitted  = UINT64_C(2) << 2,
};

struct OsTransactionCreateInfo {
    const char *NameFront;
    const char *NameBack;
    OsTransactionMode Mode;
};

extern OsStatus OsTransactBegin(struct OsTransactionCreateInfo CreateInfo, OsTransactionHandle *OutHandle);

extern OsStatus OsTransactCommit(OsTransactionHandle Handle);

extern OsStatus OsTransactRollback(OsTransactionHandle Handle);

/// @} // group OsTransaction

#ifdef __cplusplus
}
#endif
