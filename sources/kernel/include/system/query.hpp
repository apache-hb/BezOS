#pragma once

#include <bezos/facility/process.h>

#include "system/create.hpp"

#include <stddef.h>

namespace sys2 {
    struct QueryRule {
        enum { eNameEqual, eIdEqual } type;
        union {
            ObjectName name;
            OsProcessId id;
        };
    };

    struct ProcessQueryInfo {
        size_t limit;
        OsProcessHandle *handles;
        OsProcessAccess access;

        size_t ruleCount;
        QueryRule rules[];
    };
}
