#pragma once

#include <bezos/compiler.h>
#include <bezos/status.h>

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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

struct OsGuid {
    OsByte Octets[16];
};

struct OsString {
    OsSize Size;
    OsUtf8Char Data[] OS_COUNTED_BY(Size);
};

enum {
    eOsCallFileOpen = 0x10,
    eOsCallFileClose = 0x11,
    eOsCallFileRead = 0x12,
    eOsCallFileWrite = 0x13,
    eOsCallFileSeek = 0x14,
    eOsCallFileStat = 0x15,
    eOsCallFileControl = 0x16,

    eOsCallDirIter = 0x20,
    eOsCallDirNext = 0x21,

    eOsCallProcessGetCurrent = 0x30,
    eOsCallProcessCreate = 0x31,
    eOsCallProcessExit = 0x32,

    eOsCallThreadGetCurrent = 0x40,
    eOsCallThreadCreate = 0x41,
    eOsCallThreadControl = 0x42,
    eOsCallThreadDestroy = 0x43,

    eOsCallTransactionBegin = 0x50,
    eOsCallTransactionCommit = 0x51,
    eOsCallTransactionRollback = 0x52,

    eOsCallQueryParam = 0x60,
    eOsCallUpdateParam = 0x61,

    eOsCallMutexCreate = 0x70,
    eOsCallMutexDestroy = 0x71,
    eOsCallMutexLock = 0x72,
    eOsCallMutexUnlock = 0x73,

    eOsCallDebugLog = 0x90,

    eOsCallCount = 0x100,
};

struct OsCallResult {
    OsStatus Status;
    uint64_t Value;
};

/// rax:rdx - Status:Value
/// rdi - Call
/// rsi - Arg0
/// rdx - Arg1
/// rcx - Arg2
/// r8  - Arg3
extern struct OsCallResult OsSystemCall(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

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

/// @defgroup OsFile Files
/// @{

/// @brief An invalid file handle.
#define OS_FILE_INVALID ((OsFileHandle)(UINT64_MAX))

/// @brief The path separator used by the operating system.
#define OS_PATH_SEPARATOR '\0'

/// @brief Seek modes.
///
/// Bits [0:2] contain the seek mode.
/// Bits [3:63] are reserved and must be zero.
typedef uint64_t OsSeekMode;

/// @brief Open file modes.
///
/// Bits [0:7] contain the access mode.
/// Bits [8:15] contain the creation mode.
/// Bits [16:63] are reserved and must be zero.
typedef uint64_t OsFileOpenMode;

enum {
    /// @brief Open a file for reading.
    eOsFileRead = (UINT64_C(1) << 0),

    /// @brief Open a file for writing.
    eOsFileWrite = (UINT64_C(1) << 1),

    /// @brief Open a file in append mode.
    /// This implies @a OsFileWrite.
    eOsFileAppend = (UINT64_C(1) << 2),

    /// @brief Truncate the file upon opening.
    /// This implies @a OsFileWrite.
    eOsFileTruncate = (UINT64_C(1) << 3),


    /// @brief Always create a new file.
    /// If the file already exists, it is truncated.
    /// If the file does not exist, then it is created.
    eOsFileCreateAlways = (UINT64_C(1) << 8),

    /// @brief Open an existing file.
    /// If the file does not exist, then the operation fails.
    /// If the file exists, then it is opened.
    eOsFileOpenExisting = (UINT64_C(2) << 8),

    /// @brief Open a file if it exists.
    /// If the file does not exist, then it is created.
    /// If the file exists, then it is opened.
    eOsFileOpenAlways = (UINT64_C(3) << 8),
};

enum {
    /// @brief Seek relative to the beginning of the file.
    eOsSeekAbsolute = UINT64_C(0),

    /// @brief Seek relative to the current position.
    eOsSeekRelative = UINT64_C(1),

    /// @brief Seek relative to the end of the file.
    eOsSeekEnd      = UINT64_C(2),
};

/// @brief A path on the filesystem.
///
/// A path is a sequence of segments separated by @a OS_PATH_SEPARATOR.
struct OsPath {
    const OsUtf8Char *PathFront;
    const OsUtf8Char *PathBack;
};

struct OsFileCreateInfo {
    const char *PathFront;
    const char *PathBack;
};

/// @brief Open or create a file.
///
/// @pre @p PathFront and @p PathBack must point to the front and back of a valid buffer.
/// @pre @p PathBack must be > @p PathFront.
/// @pre @p OutHandle must point to valid memory.
/// @pre @p Mode must be a valid combination of @a OsFileOpenMode flags.
///
/// @post @p OutHandle will be a valid file handle if the operation is successful.
/// @post @p OutHandle will be @a OsFileHandleInvalid if the operation fails.
///
/// @param PathFront The front of the path.
/// @param PathBack The back of the path.
/// @param Mode The mode to open the file in.
/// @param OutHandle The file handle.
///
/// @return The status of the operation.
extern OsStatus OsFileOpen(const char *PathFront, const char *PathBack, OsFileOpenMode Mode, OsFileHandle *OutHandle);

/// @brief Close a file handle.
///
/// @pre @p Handle must be a valid file handle.
///
/// @param Handle The file handle to close.
///
/// @return The status of the operation.
extern OsStatus OsFileClose(OsFileHandle Handle);

/// @brief Read from a file.
///
/// @pre @p Handle must be a valid file handle.
/// @pre @p BufferFront and @p BufferBack must point to the front and back of a valid buffer.
/// @pre @p BufferBack must be > @p BufferFront.
/// @pre @p OutRead must point to valid memory.
///
/// @post @p OutRead will contain the number of bytes read.
/// @post @p BufferFront to *@p OutRead will contain the data read, the remaining data is undefined.
///
/// @param Handle The file handle to read from.
/// @param BufferFront The front of the buffer to read into.
/// @param BufferBack The back of the buffer to read into.
/// @param OutRead The number of bytes read.
///
/// @return The status of the operation.
extern OsStatus OsFileRead(OsFileHandle Handle, void *BufferFront, void *BufferBack, uint64_t *OutRead);

/// @brief Write to a file.
///
/// @pre @p Handle must be a valid file handle.
/// @pre @p BufferFront and @p BufferBack must point to the front and back of a valid buffer.
/// @pre @p BufferBack must be > @p BufferFront.
/// @pre @p OutWritten must point to valid memory.
///
/// @post @p OutWritten will contain the number of bytes written.
///
/// @param Handle The file handle to write to.
/// @param BufferFront The front of the buffer to write from.
/// @param BufferBack The back of the buffer to write from.
/// @param OutWritten The number of bytes written.
///
/// @return The status of the operation.
extern OsStatus OsFileWrite(OsFileHandle Handle, const void *BufferFront, const void *BufferBack, uint64_t *OutWritten);

/// @brief Seek to a position in a file.
///
/// @pre @p Handle must be a valid file handle.
/// @pre @p Mode must be a valid seek mode.
/// @pre @p OutPosition must point to valid memory.
///
/// @post @p OutPosition will contain the new position in the file.
///
/// @param Handle The file handle to seek in.
/// @param Mode The seek mode.
/// @param Offset The offset to seek to.
/// @param OutPosition The new position in the file.
///
/// @return The status of the operation.
extern OsStatus OsFileSeek(OsFileHandle Handle, OsSeekMode Mode, int64_t Offset, uint64_t *OutPosition);

/// @brief Perform a control operation on a file.
///
/// @pre @p Handle must be a valid file handle.
/// @pre @p ControlData must point to valid memory.
/// @pre @p ControlSize must be the size of the control data.
///
/// @param Handle The file handle to control.
/// @param ControlCode The control code.
/// @param ControlData The control data.
/// @param ControlSize The size of the control data.
///
/// @return The status of the operation.
extern OsStatus OsFileControl(OsFileHandle, uint64_t ControlCode, void *ControlData, size_t ControlSize);

/// @} // group OsFile




/// @defgroup OsFolderIterator Folder Iterator
/// @{

typedef uint16_t OsFolderEntryType;
typedef uint16_t OsFolderEntryNameSize;

enum {
    eOsNodeFile   = UINT16_C(1),
    eOsNodeFolder = UINT16_C(2),
    eOsNodeLink   = UINT16_C(3),
};

struct OsFolderEntry {
    OsFolderEntryType Type;
    OsFolderEntryNameSize NameSize;

    OsUtf8Char Name[];
};

#define OS_FOLDER_ITERATOR_INVALID ((OsFolderIteratorHandle)(0))

/// @brief Create a directory iterator.
///
/// @pre @p PathFront and @p PathBack must point to the front and back of a valid buffer.
/// @pre @p PathBack must be > @p PathFront.
/// @pre @p OutHandle must point to valid memory.
///
/// @post @p OutHandle will be a valid folder iterator handle if the operation is successful.
/// @post @p OutHandle will be @a OsFolderIteratorHandleInvalid if the operation fails.
///
/// @param PathFront The front of the path.
/// @param PathBack The back of the path.
/// @param OutHandle The folder iterator handle.
///
/// @return The status of the operation.
extern OsStatus OsFolderIterate(const char *PathFront, const char *PathBack, OsFolderIteratorHandle *OutHandle);

/// @brief Iterate to the next entry in the directory.
///
/// @pre @p Handle must be a valid folder iterator handle.
/// @pre @p OutEntry must point to valid memory.
///
/// @post @p OutEntry will contain the next entry in the directory.
///
/// @param Handle The folder iterator handle.
/// @param OutEntry The next entry in the directory.
///
/// @return The status of the operation.
extern OsStatus OsFolderIterateNext(OsFolderIteratorHandle Handle, struct OsFolderEntry *OutEntry);

/// @brief End the directory iteration.
///
/// @pre @p Handle must be a valid folder iterator handle.
///
/// @param Handle The folder iterator handle.
///
/// @return The status of the operation.
extern OsStatus OsFolderIterateEnd(OsFolderIteratorHandle Handle);

/// @} // group OsFolderIterator

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




/// @defgroup OsParam Params
/// @{

/// @brief The type of the param.
typedef uint32_t OsParamType;

/// @brief Additional flags for the param.
/// Bits [0:7] contain the field type.
/// Bits [8:12] are reserved and must be zero.
/// Bit [13] is set if an error occurred while reading the param.
/// Bit [14] is set if the param is read only.
/// Bit [15] is set if the param is an array.
/// Bits [16:31] is the size of the array if the array flag is set. Otherwise it is reserved.
typedef uint32_t OsParamFlags;

#define OS_PARAM_TYPE(param) (param & 0x000000FF)
#define OS_PARAM_ARRAY_SIZE(param) (param >> 16)

/// @brief The value of the param.
typedef uint64_t OsParamValue;

enum {
    /// @brief The param is a void type.
    /// This type has no associated value.
    eOsParamVoid     = UINT32_C(0),

    /// @brief The param is a boolean type.
    /// The associated value is of type @a OsBool.
    eOsParamBoolean  = UINT32_C(1),

    /// @brief The param is a signed integer type.
    /// The associated value is of type @a OsSigned.
    eOsParamSigned   = UINT32_C(2),

    /// @brief The param is an unsigned integer type.
    /// The associated value is of type @a OsUnsigned.
    eOsParamUnsigned = UINT32_C(3),

    /// @brief The param is a float type.
    /// The associated value is of type @a OsFloat.
    eOsParamFloat    = UINT32_C(4),

    /// @brief The param is a double type.
    /// The associated value is of type @a OsDouble.
    eOsParamDouble   = UINT32_C(5),

    /// @brief The param is a string type.
    /// The associated value is of type @a OsString.
    eOsParamString   = UINT32_C(6),

    /// @brief The param is a character type.
    /// The associated value is of type @a OsUtf8Char.
    eOsParamChar     = UINT32_C(7),

    /// @brief The param is a pointer type.
    /// The associated value is of type @a OsAnyPointer.
    eOsParamPointer  = UINT32_C(8),

    /// @brief The parameter is an array of values.
    /// The associated value is a pointer of type @a OsAnyPointer to the first element of the array.
    eOsParamFlagArray    = UINT32_C(1) << 13,

    /// @brief The parameter is read only.
    eOsParamFlagConst    = UINT32_C(1) << 14,

    /// @brief An error occurred while reading the parameter.
    eOsParamFlagError    = UINT32_C(1) << 15,
};

enum {
    eOsParamPowerState       = UINT32_C(0),
    eOsParamPhysicalMemory   = UINT32_C(1),
    eOsParamFileNameLength   = UINT32_C(2),
    eOsParamPathLength       = UINT32_C(3),
    eOsParamPageSize         = UINT32_C(4),
    eOsParamObjectNameLength = UINT32_C(5),
};

struct OsParamConfig {
    OsParamType Type;
    OsParamFlags Flags;
    OsParamValue Value;
};

/// @brief Query params for the current process and system.
///
/// @pre @p ConfigFront and @p ConfigBack must point to the front and back of a valid buffer.
/// @pre @p ConfigBack must be > @p ConfigFront.
/// @pre @p OutCount must point to valid memory.
///
/// @post @p OutCount will contain the number of params returned.
///
/// @param ConfigFront The front of the configuration buffer.
/// @param ConfigBack The back of the configuration buffer.
///
/// @return The status of the operation.
extern OsStatus OsParamSelect(struct OsParamConfig *ConfigFront, struct OsParamConfig *ConfigBack);

extern OsStatus OsParamUpdate(struct OsParamConfig *ConfigFront, struct OsParamConfig *ConfigBack);

/// @} // group OsParam

/// @defgroup OsProcess Process
/// @{

enum OsProcessCreateFlags {
    eOsProcessNone = 0,

    eOsProcessSuspended = (1 << 0),
};

struct OsProcessCreateInfo {
    const char *NameFront;
    const char *NameBack;

    const char *ArgumentsFront;
    const char *ArgumentsBack;

    uint32_t Flags;
};

extern OsStatus OsProcessCurrent(OsProcessHandle *OutHandle);

extern OsStatus OsProcessCreate(struct OsProcessCreateInfo CreateInfo, OsProcessHandle *OutHandle);

extern OsStatus OsProcessSuspend(OsProcessHandle Handle, bool Suspend);

extern OsStatus OsProcessTerminate(OsProcessHandle Handle);

/// @} // group OsProcess

/// @defgroup OsThread Thread
/// @{

enum OsThreadCreateFlags {
    eOsThreadNone = 0,

    eOsThreadSuspended = (1 << 0),
};

struct OsThreadCreateInfo {
    const char *NameFront;
    const char *NameBack;

    OsProcessHandle Process;
    OsSize StackSize;
    OsAnyPointer EntryPoint;
    OsAnyPointer Argument;
    uint32_t Flags;
};

extern OsStatus OsThreadCurrent(OsThreadHandle *OutHandle);

extern OsStatus OsThreadYield(void);

extern OsStatus OsThreadSleep(OsDuration Duration);

extern OsStatus OsThreadCreate(struct OsThreadCreateInfo CreateInfo, OsThreadHandle *OutHandle);

extern OsStatus OsThreadSuspend(OsThreadHandle Handle, bool Suspend);

extern OsStatus OsThreadDestroy(OsThreadHandle Handle);

/// @} // group OsThread

/// @defgroup OsMutex Mutex
/// @{

struct OsMutexCreateInfo {
    const char *NameFront;
    const char *NameBack;
};

extern OsStatus OsMutexCreate(struct OsMutexCreateInfo CreateInfo, OsMutexHandle *OutHandle);

extern OsStatus OsMutexLock(OsMutexHandle Handle);

extern OsStatus OsMutexUnlock(OsMutexHandle Handle);

extern OsStatus OsMutexDestroy(OsMutexHandle Handle);

/// @} // group OsMutex

/// @defgroup OsAddressSpace Address Space
/// @{

enum {
    eOsMemoryNoAccess = 0,
    eOsMemoryRead     = (1 << 0),
    eOsMemoryWrite    = (1 << 1),
    eOsMemoryExecute  = (1 << 2),
};

struct OsAddressSpaceCreateInfo {
    const char *NameFront;
    const char *NameBack;

    OsSize Size;
    uint32_t Flags;
};

extern OsStatus OsAddressSpaceCreate(struct OsAddressSpaceCreateInfo CreateInfo, OsAddressSpaceHandle *OutHandle);

/// @} // group OsAddressSpaceHandle

#ifdef __cplusplus
}
#endif
