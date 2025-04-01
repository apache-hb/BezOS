#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @defgroup OsTransaction Transactions
/// @{

enum {
    eOsTxAccessNone = 0,

    /// @brief Access to wait for the transaction to complete.
    eOsTxAccessWait = (1 << 0),

    /// @brief Access to commit the transaction.
    eOsTxAccessCommit = (1 << 1),

    /// @brief Access to rollback the transaction.
    eOsTxAccessRollback = (1 << 2),

    /// @brief Access to get the transaction status.
    eOsTxAccessStat = (1 << 3),

    /// @brief Access to add new actions to the transaction.
    eOsTxAccessWrite = (1 << 4),

    eOsTxAccessAll
        = eOsTxAccessWait
        | eOsTxAccessCommit
        | eOsTxAccessRollback
        | eOsTxAccessStat
        | eOsTxAccessWrite,
};

/// @brief Transaction creation modes.
///
/// Bits [0:1] contain the read transaction mode.
/// Bits [2:3] contain the write transaction mode.
/// Bits [4:63] are reserved and must be zero.
typedef uint64_t OsTxMode;

enum {
    eOsIsolateReadSerializable = UINT64_C(0),
    eOsIsolateReadCommitted    = UINT64_C(1),
    eOsIsolateReadUncommitted  = UINT64_C(2),

    eOsIsolateWriteSerializable = UINT64_C(0) << 2,
    eOsIsolateWriteCommitted    = UINT64_C(1) << 2,
    eOsIsolateWriteUncommitted  = UINT64_C(2) << 2,
};

struct OsTxCreateInfo {
    const char *NameFront;
    const char *NameBack;
    OsTxMode Mode;

    OsProcessHandle Process;
    OsTxHandle Transaction;
};

struct OsTxInfo {
    OsUtf8Char Name[OS_OBJECT_NAME_MAX];
    OsProcessHandle Process;
    OsTxHandle Parent;
};

extern OsStatus OsTxBegin(struct OsTxCreateInfo CreateInfo, OsTxHandle *OutHandle);

extern OsStatus OsTxCommit(OsTxHandle Handle);

extern OsStatus OsTxRollback(OsTxHandle Handle);

extern OsStatus OsTxStat(OsTxHandle Handle, struct OsTxInfo *OutInfo);

/// @} // group OsTransaction

#ifdef __cplusplus
}
#endif
