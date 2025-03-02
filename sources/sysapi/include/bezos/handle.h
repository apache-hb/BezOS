#pragma once

#include <bezos/compiler.h>
#include <bezos/status.h>

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OS_DEVICE_NAME_MAX 128
#define OS_OBJECT_NAME_MAX 128

#define OS_OBJECT_HANDLE(name) typedef OsHandle name

typedef uint8_t OsByte;
typedef size_t OsSize;
typedef char OsUtf8Char;

typedef uint64_t OsHandle;
typedef bool OsBool;
typedef uint64_t OsUnsigned;
typedef int64_t OsSigned;
typedef float OsFloat;
typedef double OsDouble;
typedef void *OsAnyPointer;
typedef uint64_t OsDuration;
typedef uint64_t OsInstant;

struct OsString {
    OsSize Size;
    OsUtf8Char Data[] OS_COUNTED_BY(Size);
};

/// @brief A path in the filesystem.
///
/// A path is a sequence of segments separated by @a OS_PATH_SEPARATOR.
struct OsPath {
    const OsUtf8Char *Front;
    const OsUtf8Char *Back;
};

struct OsGuid {
    OsByte Octets[16];
};

/// @brief A version tag.
///
/// All versioning is done using a single 32-bit integer and semantic versioning.
/// The version is stored as follows:
/// - Bits 31-24: Major version.
/// - Bits 23-16: Minor version.
/// - Bits 15-0: Patch version.
///
/// @see OS_VERSION_MAJOR
/// @see OS_VERSION_MINOR
/// @see OS_VERSION_PATCH
/// @see OS_VERSION
typedef uint32_t OsVersionTag;

#define OS_VERSION_MAJOR(version) (((OsVersionTag)(version) & 0xff000000) >> 24)
#define OS_VERSION_MINOR(version) (((OsVersionTag)(version) & 0x00ff0000) >> 16)
#define OS_VERSION_PATCH(version) (((OsVersionTag)(version) & 0x0000ffff))

#define OS_VERSION(major, minor, patch) \
    (((OsVersionTag)(major) << 24) | ((OsVersionTag)(minor) << 16) | (OsVersionTag)(patch))

#define OS_DEFINE_GUID(NAME, P0, P1, P2, P3, P4) \
    static const struct OsGuid NAME = { \
        .Octets = { \
            ((P0 >> 24) & 0xff), ((P0 >> 16) & 0xff), ((P0 >> 8) & 0xff), (P0 & 0xff), \
            ((P1 >> 8) & 0xff), (P1 & 0xff), \
            ((P2 >> 8) & 0xff), (P2 & 0xff), \
            ((P3 >> 8) & 0xff), (P3 & 0xff), \
            ((P4 >> 40) & 0xff), ((P4 >> 32) & 0xff), ((P4 >> 24) & 0xff), ((P4 >> 16) & 0xff), ((P4 >> 8) & 0xff), (P4 & 0xff), \
        } \
    }

#define OS_TIMEOUT_INFINITE ((OsInstant)(UINT64_MAX))
#define OS_HANDLE_INVALID ((OsHandle)(0))

enum {
    eOsHandleUnknown      = UINT8_C(0x0),
    eOsHandleNode         = UINT8_C(0x1),
    eOsHandleMutex        = UINT8_C(0x2),
    eOsHandleThread       = UINT8_C(0x3),
    eOsHandleProcess      = UINT8_C(0x4),
    eOsHandleDevice       = UINT8_C(0x5),
    eOsHandleAddressSpace = UINT8_C(0x6),
};

typedef uint8_t OsHandleType;

/// @brief A handle to a file object.
OS_OBJECT_HANDLE(OsFileHandle);

/// @brief A handle to a folder iterator.
OS_OBJECT_HANDLE(OsFolderIteratorHandle);

/// @brief A handle to a transaction.
OS_OBJECT_HANDLE(OsTransactionHandle);

/// @brief A handle to a thread.
OS_OBJECT_HANDLE(OsThreadHandle);

/// @brief A handle to a mutex.
OS_OBJECT_HANDLE(OsMutexHandle);

/// @brief A handle to a process.
OS_OBJECT_HANDLE(OsProcessHandle);

/// @brief A handle to an address space.
OS_OBJECT_HANDLE(OsAddressSpaceHandle);

/// @brief A handle to a device.
OS_OBJECT_HANDLE(OsDeviceHandle);

// TODO: remove this
extern void OsDebugLog(const char *Begin, const char *End);

#ifdef __cplusplus
}
#endif
