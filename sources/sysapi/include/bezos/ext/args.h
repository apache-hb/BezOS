#pragma once

#include <bezos/facility/process.h>

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

inline OsStatus OsProcessFindArg(const struct OsProcessInfo *Info, const struct OsGuid *Guid, const struct OsProcessParam **OutParam) {
    if (Info->ArgsBegin == Info->ArgsEnd) {
        return OsStatusNotFound;
    }

    const struct OsProcessParam *param = Info->ArgsBegin;
    while (param < Info->ArgsEnd) {
        if (memcmp(&param->Guid, Guid, sizeof(*Guid)) == 0) {
            *OutParam = param;
            return OsStatusSuccess;
        }

        param = (const struct OsProcessParam*)((uintptr_t)param + param->DataSize + sizeof(struct OsProcessParam));
    }

    return OsStatusNotFound;
}

#ifdef __cplusplus
}
#endif
