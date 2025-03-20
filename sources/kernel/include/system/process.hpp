#pragma once

#include "std/static_string.hpp"

namespace sys2 {
    class Process {

    };

    struct ProcessCreateInfo {
        Process *parent;
        stdx::StaticString<32> name;
    };
}
