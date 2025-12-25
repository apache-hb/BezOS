#include "pkgtool/proc_mounts.hpp"

#include <sstream>
#include <fstream>
#include <fstab.h>

pkg::ProcMounts::ProcMounts(std::istream& is) {
    std::string line;
    while (std::getline(is, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream lineStream(line);
        MountEntry entry;
        lineStream >> entry.device >> entry.mount >> entry.fstype >> entry.options;
        mEntries.push_back(std::move(entry));
    }
}

pkg::ProcMounts pkg::ProcMounts::ofCurrentMachine() {
    std::ifstream file{"/proc/mounts"};
    return ProcMounts{file};
}
