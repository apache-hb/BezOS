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

    /// @brief Reserve virtual address space
    /// Reserved space is not accessible, but can be committed later.
    eOsMemoryReserve  = (1 << 4),

    /// @brief Commit virtual address space
    /// Committed space is accessible and can be read or written.
    eOsMemoryCommit   = (1 << 5),

    /// @brief Discard a range of memory.
    /// Discarded memory remains committed, and will be zeroed on first access.
    /// Combined with @c eOsMemoryCommit this will zero newly committed memory.
    eOsMemoryDiscard  = (1 << 6),

    /// @brief Is the base address a hint.
    /// If this is set, the base address is a hint and the system will choose the address.
    /// Otherwise, the base address is the address to map the memory at.
    eOsMemoryAddressHint = (1 << 8),
};

/// @brief Memory access flags.
///
/// Bits [0:2] contain the memory access flags.
/// Bits [3:7] contain the memory allocation flags.
/// Bit  [8] controls whether the base address is a hint.
/// Bits [9:31] are reserved and must be zero.
typedef uint32_t OsMemoryAccess;

struct OsVmemMapInfo {
    /// @brief The address from the source object to map.
    OsAddress SrcAddress;

    /// @brief The address in the destination process to map the memory at.
    OsAddress DstAddress;

    /// @brief The amount of memory to map.
    OsSize Size;

    /// @brief The access flags the memory should be mapped with.
    OsMemoryAccess Access;

    /// @brief The source object to map the memory from.
    OsHandle Source;

    /// @brief The destination object to map the memory to.
    OsProcessHandle Process;

    /// @brief The transaction this operation is associated with.
    OsTxHandle Transaction;
};

struct OsVmemCreateInfo {
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

    /// @brief The transaction this address space is associated with.
    /// If this is @a OS_HANDLE_INVALID, then the address space operation is not isolated.
    OsTxHandle Transaction;
};

/// @brief Extend the address space of a process.
///
/// @param CreateInfo The create information for the address space.
/// @param OutBase The base address of the address space.
///
/// @return The status of the operation.
extern OsStatus OsVmemCreate(struct OsVmemCreateInfo CreateInfo, OsAnyPointer *OutBase);

/// @brief Alias memory from one objects address space to another.
///
/// @param MapInfo The mapping information.
/// @param OutBase The base address of the mapped memory.
///
/// @return The status of the operation.
extern OsStatus OsVmemMap(struct OsVmemMapInfo MapInfo, OsAnyPointer *OutBase);

/// @brief Unmap a range of memory.
///
/// @param BaseAddress The base address of the memory to unmap.
/// @param Size The size of the memory to unmap.
///
/// @return The status of the operation.
extern OsStatus OsVmemRelease(OsAnyPointer BaseAddress, OsSize Size);

/// @} // group OsAddressSpace

#ifdef __cplusplus
}
#endif
