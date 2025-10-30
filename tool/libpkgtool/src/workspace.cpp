#include "pkgtool/pkgtool.hpp"

#include <map>

using pkg::IWorkspace;

namespace fs = std::filesystem;

class WorkspaceImpl final : public IWorkspace {
    fs::path mRoot;
    std::map<std::string, std::shared_ptr<pkg::IPackage>> mPackages;
public:
    WorkspaceImpl(const fs::path& root)
        : mRoot(root)
    {
        auto sources = root / "sources";

        for (const auto& entry : fs::directory_iterator(sources)) {
            if (entry.is_directory()) {
                mPackages[entry.path().filename().string()] = pkg::IPackage::of(entry);
            } else {
                throw std::runtime_error("Unexpected file in sources folder: " + entry.path().string());
            }
        }
    }
};

std::shared_ptr<IWorkspace> IWorkspace::ofRootPath(const fs::path& root) {
    return std::make_shared<WorkspaceImpl>(root);
}
