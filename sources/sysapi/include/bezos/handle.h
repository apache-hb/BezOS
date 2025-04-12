#pragma once

#include <bezos/compiler.h>
#include <bezos/status.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OS_DEVICE_NAME_MAX 128
#define OS_OBJECT_NAME_MAX 255

#define OS_OBJECT_HANDLE(name) typedef OsHandle name

typedef uint8_t OsByte;
typedef size_t OsSize;
typedef char OsUtf8Char;
typedef bool OsBool;
typedef uint64_t OsUnsigned;
typedef int64_t OsSigned;
typedef float OsFloat;
typedef double OsDouble;
typedef void *OsAnyPointer;
typedef uintptr_t OsAddress;

/// @brief A duration of time.
///
/// Duration is measured in nanoseconds and can be used for timeouts.
typedef uint64_t OsDuration;

/// @brief A point in time.
///
/// The time is represented as the number of 100 nanosecond intervals since
/// the gregorian calendar reform of 1582-10-15T00:00:00Z.
typedef int64_t OsInstant;

/// @brief A tick of a time source.
typedef uint64_t OsTickCounter;

/// @brief A generic handle.
///
/// A handle is a unique identifier for a resource that is managed by the operating system.
/// The handle contains a type and a unique identifier.
/// - bits [0:55] contain the unique identifier.
/// - bits [56:63] contain the type as specified by @a OsHandleType.
///
/// @see OsHandleType
typedef uint64_t OsHandle;

typedef uint64_t OsHandleAccess;

#define OS_HANDLE_TYPE(handle) ((OsHandle)((handle) >> 56))
#define OS_HANDLE_ID(handle) ((handle) & 0x00ffffffffffffff)
#define OS_HANDLE_NEW(type, id) (((OsHandle)(type) << 56) | (OsHandle)(id))

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

struct OsBuffer {
    const void *Data;
    OsSize Size;
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

#if defined(__cplusplus) && __cplusplus >= 201402L
#   define OS_CXX_CONSTEXPR constexpr
#else
#   define OS_CXX_CONSTEXPR const
#endif

#define OS_DEFINE_GUID(NAME, P0, P1, P2, P3, P4) \
    static OS_CXX_CONSTEXPR struct OsGuid NAME = { \
        .Octets = { \
            ((P0 >> 24) & 0xff), ((P0 >> 16) & 0xff), ((P0 >> 8) & 0xff), (P0 & 0xff), \
            ((P1 >> 8) & 0xff), (P1 & 0xff), \
            ((P2 >> 8) & 0xff), (P2 & 0xff), \
            ((P3 >> 8) & 0xff), (P3 & 0xff), \
            ((P4 >> 40) & 0xff), ((P4 >> 32) & 0xff), ((P4 >> 24) & 0xff), ((P4 >> 16) & 0xff), ((P4 >> 8) & 0xff), (P4 & 0xff), \
        } \
    }

/// @brief Used to indicate that a timeout should be infinite.
#define OS_TIMEOUT_INFINITE ((OsInstant)(0))

/// @brief Used to indicate that a timeout should be instant, i.e. non-blocking.
#define OS_TIMEOUT_INSTANT ((OsInstant)(INT64_MAX))

#define OS_HANDLE_INVALID ((OsHandle)(0))

enum {
    eOsHandleUnknown      = UINT8_C(0x0),

    /// @brief Handle to a vnode object in the vfs.
    eOsHandleNode         = UINT8_C(0x1),

    /// @brief Handle to an instance of a device interface on a vnode.
    eOsHandleDevice       = UINT8_C(0x2),

    /// @brief Handle to a mutex object.
    eOsHandleMutex        = UINT8_C(0x3),

    /// @brief Handle to a thread object.
    eOsHandleThread       = UINT8_C(0x4),

    /// @brief Handle to a process object.
    eOsHandleProcess      = UINT8_C(0x5),

    /// @brief Handle to a transaction object.
    eOsHandleTx           = UINT8_C(0x6),

    /// @brief Handle to an event object.
    eOsHandleEvent        = UINT8_C(0x7),

    eOsHandleCount,
};

typedef uint8_t OsHandleType;

/// @brief A handle to a vnode.
OS_OBJECT_HANDLE(OsNodeHandle);

/// @brief A handle to a device.
OS_OBJECT_HANDLE(OsDeviceHandle);

/// @brief A handle to a mutex.
OS_OBJECT_HANDLE(OsMutexHandle);

/// @brief A handle to a thread.
OS_OBJECT_HANDLE(OsThreadHandle);

/// @brief A handle to a process.
OS_OBJECT_HANDLE(OsProcessHandle);

/// @brief A handle to a transaction.
OS_OBJECT_HANDLE(OsTxHandle);

/// @brief A handle to an event.
OS_OBJECT_HANDLE(OsEventHandle);

#define OS_GUID_NULL { .Octets = { 0 } }

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus)
template<size_t N>
constexpr OsPath OsMakePath(const char (&Path)[N]) {
    const char *front = Path;
    const char *back = Path + N - 1;
    return OsPath { front, back };
}
#endif
