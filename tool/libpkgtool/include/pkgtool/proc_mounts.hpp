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

    class ProcMounts {
        std::vector<MountEntry> mEntries;
    public:
        ProcMounts(std::istream& is);

        static ProcMounts ofCurrentMachine();

        std::vector<MountEntry> entries() const {
            return mEntries;
        }
    };
}
