#pragma once

#include "rtld/arch/base/relocation.hpp"

namespace os::elf::detail {
    struct LoaderSparcV9 : public LoaderBase {
        struct Dyn {

        };

        struct Rela {

        };

        static OsStatus ApplyRelocations();
    };

    using Loader = LoaderSparcV9;
}
