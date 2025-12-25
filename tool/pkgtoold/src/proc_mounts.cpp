#include "pkgtoold/proc_mounts.hpp"

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

std::vector<pkg::OverlayMountEntry> pkg::ProcMounts::overlayEntries() const {
    std::vector<OverlayMountEntry> overlays;
    for (const auto& entry : mEntries) {
        if (entry.fstype != "overlay") {
            continue;
        }

        OverlayMountEntry overlayEntry;
        overlayEntry.overlay = entry.mount;

        // Parse options to find lowerdir, upperdir, workdir
        std::istringstream optionsStream(entry.options);
        std::string option;
        while (std::getline(optionsStream, option, ',')) {
            if (option.starts_with("lowerdir=")) {
                std::string lowersStr = option.substr(sizeof("lowerdir=") - 1);
                std::istringstream lowersStream(lowersStr);
                std::string lower;
                while (std::getline(lowersStream, lower, ':')) {
                    overlayEntry.lowers.push_back(lower);
                }
            } else if (option.starts_with("upperdir=")) {
                overlayEntry.upper = option.substr(sizeof("upperdir=") - 1);
            } else if (option.starts_with("workdir=")) {
                overlayEntry.work = option.substr(sizeof("workdir=") - 1);
            }
        }

        overlays.push_back(std::move(overlayEntry));
    }

    return overlays;
}
