#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

struct OsClientStartInfo {
    const char *ArgumentsFront;
    const char *ArgumentsBack;

    OsProcessHandle ParentProcess;
};

OS_EXTERN OS_NORETURN void ClientStart(const struct OsClientStartInfo *);

#ifdef __cplusplus
}
#endif
