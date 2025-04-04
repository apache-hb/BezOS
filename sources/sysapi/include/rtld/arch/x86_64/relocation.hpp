#pragma once

#include "rtld/arch/base/relocation.hpp"

namespace os::elf::detail {
    struct LoaderX86_64 : public LoaderBase {
        struct Dyn {
            uint64_t tag;
            uint64_t value;
        };

        struct Rela {
            uint64_t offset;
            uint64_t info;
            int64_t addend;
        };

        static OsStatus ApplyRelocations();
    };

    using Loader = LoaderX86_64;
}
