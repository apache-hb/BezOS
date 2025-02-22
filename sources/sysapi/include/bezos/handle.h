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

struct OsGuid {
    OsByte Octets[16];
};

#define OS_DEFINE_GUID(NAME, P0, P1, P2, P3, P4) \
    static const inline OsGuid NAME = { \
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
