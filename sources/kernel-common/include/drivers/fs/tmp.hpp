#pragma once

#include "drivers/fs/driver.hpp"

namespace km {
    class TmpFileSystem : public IFileSystem {
    public:
        TmpFileSystem() = default;
    };
}
