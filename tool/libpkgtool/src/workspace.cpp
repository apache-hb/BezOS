#include "pkgtool/pkgtool.hpp"

#include <map>

#include "xml.hpp"

using pkg::IWorkspace;

namespace fs = std::filesystem;

namespace {

class WorkspaceImpl final : public IWorkspace {
    fs::path mRoot;
    std::map<std::string, std::shared_ptr<pkg::IPackage>> mPackages;
public:
    WorkspaceImpl(const fs::path& root)
        : mRoot(root)
    {
        auto workspace = mRoot / "workspace.xml";
        auto doc = XmlDocument::parse(workspace);

        auto node = doc.root();
        if (node.name() != "workspace") {
            throw std::runtime_error(std::format("ERROR [{}:{}]: Invalid root element <{}> in {}, expected <workspace>", node.path(), node.line(), node.name(), workspace.string()));
        }

        for (const auto& child : node.children()) {
            if (child.name() != "package") {
                throw std::runtime_error(std::format("ERROR [{}:{}]: Unexpected element <{}> in {}, expected <package>", child.path(), child.line(), child.name(), workspace.string()));
            }

            auto inner = child.expect("path");
            auto package = pkg::IPackage::of(root / inner);

            mPackages.emplace(package->name(), package);
        }
    }

    std::shared_ptr<pkg::IPackage> package(std::string_view name) const override {
        return mPackages.at(std::string{name});
    }

    std::map<std::string, std::shared_ptr<pkg::IPackage>> packages() const override {
        return mPackages;
    }
};

}

std::shared_ptr<IWorkspace> IWorkspace::ofRootPath(const fs::path& root) {
    return std::make_shared<WorkspaceImpl>(root);
}
