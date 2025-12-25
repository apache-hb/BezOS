#pragma once

#include <istream>
#include <vector>

namespace pkg {
    struct MountEntry {
        std::string device;
        std::string mount;
        std::string fstype;
        std::string options;
    };

    struct OverlayMountEntry {
        std::string overlay;
        std::string upper;
        std::string work;
        std::vector<std::string> lowers;
    };

    class ProcMounts {
        std::vector<MountEntry> mEntries;
    public:
        ProcMounts(std::istream& is);

        static ProcMounts ofCurrentMachine();

        std::vector<MountEntry> entries() const {
            return mEntries;
        }

        std::vector<OverlayMountEntry> overlayEntries() const;
    };
}
