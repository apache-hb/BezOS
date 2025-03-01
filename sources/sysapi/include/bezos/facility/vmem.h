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
};

/// @brief Memory access flags.
///
/// Bits [0:2] contain the memory access flags.
/// Bits [3:31] are reserved and must be zero.
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

struct OsAddressSpaceInfo {
    OsAnyPointer Base;
    OsSize Size;
    OsMemoryAccess Access;
};

extern OsStatus OsAddressSpaceCreate(struct OsAddressSpaceCreateInfo CreateInfo, OsAddressSpaceHandle *OutHandle);

extern OsStatus OsAddressSpaceDestroy(OsAddressSpaceHandle Handle);

extern OsStatus OsAddressSpaceStat(OsAddressSpaceHandle Handle, struct OsAddressSpaceInfo *OutStat);

/// @} // group OsAddressSpaceHandle

#ifdef __cplusplus
}
#endif
