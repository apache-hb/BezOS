#include "pkgtoold/pkgtoold.hpp"

#include <fstream>

bool pkg::hostHasOverlayFsSupport() {
    std::ifstream filesystems{"/proc/filesystems"};
    std::string line;
    while (std::getline(filesystems, line)) {
        if (line.find("overlay") != std::string::npos) {
            return true;
        }
    }

    return false;
}

std::string pkg::pkgtooldUnixSocketPath() {
    return "/var/run/pkgtoold.sock";
}
