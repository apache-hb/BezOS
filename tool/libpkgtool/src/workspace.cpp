#include "pkgtool/pkgtool.hpp"

#include <map>
#include <print>

#include "xml.hpp"

using pkg::IWorkspace;

namespace fs = std::filesystem;

namespace {

class WorkspaceImpl final : public IWorkspace {
    fs::path mRoot;
    std::map<std::string, std::shared_ptr<pkg::IPackage>> mPackages;
public:
    WorkspaceImpl(const fs::path& root)
        : mRoot(root.parent_path())
    {
        auto doc = XmlDocument::parse(root);

        auto node = doc.root();
        if (node.name() != "workspace") {
            throw std::runtime_error(std::format("ERROR [{}:{}]: Invalid root element <{}> in {}, expected <workspace>", node.path(), node.line(), node.name(), mRoot.string()));
        }

        for (const auto& child : node.children()) {
            if (child.name() == "text" || child.name() == "comment") {
                continue;
            }

            if (child.name() != "package") {
                throw std::runtime_error(std::format("ERROR [{}:{}]: Unexpected element {} in {}, expected <package>", child.path(), child.line(), child.name(), mRoot.string()));
            }

            auto inner = child.expect("path");

            auto path = mRoot / inner;
            try {
                auto package = pkg::IPackage::of(path);
                mPackages.emplace(package->name(), package);
            } catch (const std::exception& e) {
                throw std::runtime_error(std::format("ERROR [{}:{}]: Failed to load package at {}: {}", child.path(), child.line(), (mRoot / inner).string(), e.what()));
            }
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
