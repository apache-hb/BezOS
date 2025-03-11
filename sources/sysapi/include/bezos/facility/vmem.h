#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @defgroup OsAddressSpace Address Space
/// @{

enum {
    eOsMemoryNoAccess = 0,

    eOsMemoryRead     = (1 << 0),
    eOsMemoryWrite    = (1 << 1),
    eOsMemoryExecute  = (1 << 2),

    eOsMemoryReserve  = (1 << 4),
    eOsMemoryCommit   = (1 << 5),
    eOsMemoryDiscard  = (1 << 6),
    eOsMemoryRelease  = (1 << 7),

    eOsMemoryAddressHint = (1 << 8),
};

/// @brief Memory access flags.
///
/// Bits [0:2] contain the memory access flags.
/// Bits [3:7] contain the memory allocation flags.
/// Bit [8] controls whether the base address is a hint.
/// Bits [9:31] are reserved and must be zero.
typedef uint32_t OsMemoryAccess;

struct OsAddressSpaceCreateInfo {
    const char *NameFront;
    const char *NameBack;

    /// @brief The minimum size of the address space.
    /// Must be a multiple of the page size.
    OsSize Size;

    /// @brief The alignment of the address space.
    /// Must be a multiple of the page size.
    OsSize Alignment;

    /// @brief The access flags for the address space.
    OsMemoryAccess Access;

    /// @brief The process handle to create the address space for.
    /// If this is OS_HANDLE_INVALID, then the address space is created for the current process.
    OsProcessHandle Process;
};

struct OsVmemCreateInfo {
    const char *NameFront;
    const char *NameBack;

    /// @brief The base address to map the memory at.
    /// If @c eOsMemoryAddressHint is set in the access flags, then this is a hint. If the range is already in use
    /// another address will be chosen. Otherwise, this is the base address to map the memory at. If the address is already
    /// in use the operation will fail.
    /// If this is @c NULL then the system will choose the address.
    /// @note If @c eOsMemoryAddressHint is set, then the address must not be @c NULL.
    OsAnyPointer BaseAddress;

    /// @brief The minimum size of the address space.
    /// Must be a multiple of the page size.
    OsSize Size;

    /// @brief The alignment of the address space.
    /// Must be a multiple of the page size.
    OsSize Alignment;

    /// @brief The access flags for the address space.
    OsMemoryAccess Access;

    /// @brief The process handle to create the address space for.
    /// If this is OS_HANDLE_INVALID, then the address space is created for the current process.
    OsProcessHandle Process;
};

/// @brief Extend the address space of a process.
///
/// @param CreateInfo The create information for the address space.
/// @param OutBase The base address of the address space.
///
/// @return The status of the operation.
extern OsStatus OsVmemCreate(struct OsVmemCreateInfo CreateInfo, OsAnyPointer *OutBase);

/// @} // group OsAddressSpace

#ifdef __cplusplus
}
#endif
