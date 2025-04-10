#pragma once

#include <bezos/facility/process.h>

#include "system/create.hpp"

#include <stddef.h>

namespace sys2 {
    struct ProcessQueryInfo {
        size_t limit;
        OsProcessHandle *handles;
        OsProcessAccess access;

        ObjectName matchName;
    };

    struct ProcessQueryResult {
        size_t found;
    };
}
