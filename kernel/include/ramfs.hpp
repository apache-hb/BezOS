#include "vfs.hpp"

namespace km {
    struct RamFsNode;

    struct RamFsFolder {
        BTreeMap<stdx::String, std::unique_ptr<RamFsNode>> mNodes;
    };

    struct RamFsNode {
        std::variant<RamFsFolder> mNode;
    };
}
