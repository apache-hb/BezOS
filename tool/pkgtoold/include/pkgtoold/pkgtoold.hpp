#pragma once

#include <string>

namespace pkg {
    bool hostHasOverlayFsSupport();
    std::string pkgtooldUnixSocketPath();
}
