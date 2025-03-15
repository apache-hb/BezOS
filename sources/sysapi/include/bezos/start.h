#pragma once

#include <bezos/handle.h>

#ifdef __cplusplus
extern "C" {
#endif

struct OsClientStartInfo {
    const struct OsProcessParam *ArgsBegin;
    const struct OsProcessParam *ArgsEnd;

    OsProcessHandle ThisProcess;
    OsProcessHandle ParentProcess;
};

OS_EXTERN OS_NORETURN void ClientStart(const struct OsClientStartInfo *);

#ifdef __cplusplus
}
#endif
