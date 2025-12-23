#include "pkgtool/pkgtool.hpp"

#include <map>
#include <set>

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
            throw std::runtime_error(std::format("ERROR {}: Invalid root element <{}> in {}, expected <workspace>", locationToString(node), node.name(), mRoot.string()));
        }

        for (const auto& child : node.children()) {
            if (child.name() == "text" || child.name() == "comment") {
                continue;
            }

            if (child.name() != "package") {
                throw std::runtime_error(std::format("ERROR {}: Unexpected element {} in {}, expected <package>", locationToString(node), child.name(), mRoot.string()));
            }

            auto inner = child.expect("path");

            auto path = mRoot / inner;
            try {
                auto package = pkg::IPackage::of(path, *this);
                mPackages.emplace(package->name(), package);
            } catch (const std::exception& e) {
                throw std::runtime_error(std::format("ERROR {}: Failed to load package at {}: {}", locationToString(node), (mRoot / inner).string(), e.what()));
            }
        }
    }

    std::shared_ptr<pkg::IPackage> package(std::string_view name) const override {
        return mPackages.at(std::string{name});
    }

    std::map<std::string, std::shared_ptr<pkg::IPackage>> packages() const override {
        return mPackages;
    }

    std::filesystem::path path() const override {
        return mRoot;
    }
};

}

std::vector<std::shared_ptr<pkg::IPackage>> pkg::dependencyClosure(IWorkspace& workspace, const std::string& name) {
    std::vector<std::shared_ptr<IPackage>> result;
    std::set<std::string> visited;

    const auto& packages = workspace.packages();
    auto visit = [&](this auto&& self, const std::string& name) {
        if (visited.contains(name)) {
            return;
        }

        visited.insert(name);

        if (!packages.contains(name)) {
            throw std::runtime_error("Unknown package: " + name);
        }

        auto& package = packages.at(name);
        for (const auto& dep : package->dependencies()) {
            self(dep);
        }

        result.push_back(package);
    };

    visit(name);

    return result;
}

std::shared_ptr<IWorkspace> IWorkspace::ofRootPath(const fs::path& root) {
    return std::make_shared<WorkspaceImpl>(root);
}
