#pragma once

#include "isr.hpp"
#include "std/string.hpp"

namespace km {
    class Thread {

    };

    class Process {
        stdx::String mName;
        Privilege mPrivilege;

    public:
        Process();
    };
}
