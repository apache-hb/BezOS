#pragma once

#include <bezos/status.h>

namespace os::elf::detail {
    struct LoaderBase {
        struct Dyn;
        struct Rela;

        [[gnu::error("LoaderBase::ApplyRelocations not implemented by platform")]]
        static OsStatus ApplyRelocations();
    };
}
