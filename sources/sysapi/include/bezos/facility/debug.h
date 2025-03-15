#pragma once

#include <stddef.h> // IWYU pragma: keep size_t

#include <bezos/status.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Extra information for a debug message.
///
/// bits [0:7] contain the severity of the message.
/// bits [8:63] are reserved and must be zero.
typedef uint64_t OsDebugMessageFlags;

enum {
    eOsLogDebug    = UINT64_C(0x0),
    eOsLogInfo     = UINT64_C(0x1),
    eOsLogWarning  = UINT64_C(0x2),
    eOsLogError    = UINT64_C(0x3),
    eOsLogCritical = UINT64_C(0x4),

    eOsLogMask     = UINT64_C(0xF),
};

struct OsDebugMessageInfo {
    const char *Front;
    const char *Back;
    OsDebugMessageFlags Info;
};

/// @brief Submit a message to the process log.
///
/// @param Message The message to log.
///
/// @return The status of the operation.
extern OsStatus OsDebugMessage(struct OsDebugMessageInfo Message);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
template<size_t N>
inline OsStatus OsDebugMessage(OsDebugMessageFlags Flags, const char (&Message)[N]) {
    OsDebugMessageInfo messageInfo {
        .Front = Message,
        .Back = Message + N - 1,
        .Info = Flags,
    };
    return OsDebugMessage(messageInfo);
}
#endif /* __cplusplus */
