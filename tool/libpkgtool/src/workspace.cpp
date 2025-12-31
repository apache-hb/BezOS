#include "pkgtool/pkgtool.hpp"

#include <map>
#include <set>

#include "xml.hpp"

#include <quill/Frontend.h>
#include <quill/LogMacros.h>

#include <fmt/format.h>

using pkg::IWorkspace;

namespace fs = std::filesystem;

namespace {

class WorkspaceImpl final : public IWorkspace {
    static inline auto logger() {
        static auto it = quill::Frontend::create_or_get_logger("WorkspaceImpl", quill::Frontend::get_logger("root"));
        return it;
    }

    fs::path mRoot;
    std::map<std::string, std::shared_ptr<pkg::IPackage>> mPackages;
public:
    WorkspaceImpl(const fs::path& root)
        : mRoot(root.parent_path())
    {
        auto doc = XmlDocument::parse(root);

        auto node = doc.root();
        if (node.name() != "workspace") {
            throw std::runtime_error(fmt::format("ERROR {}: Invalid root element <{}> in {}, expected <workspace>", locationToString(node), node.name(), mRoot.string()));
        }

        for (const auto& child : node.elements()) {
            if (child.name() != "package") {
                throw std::runtime_error(fmt::format("ERROR {}: Unexpected element {} in {}, expected <package>", locationToString(node), child.name(), mRoot.string()));
            }

            auto inner = child.expect("path");

            auto path = mRoot / inner;
            try {
                auto package = pkg::IPackage::of(path, *this);
                mPackages.emplace(package->name(), package);
            } catch (const std::exception& e) {
                throw std::runtime_error(fmt::format("ERROR {}: Failed to load package at {}: {}", locationToString(node), (mRoot / inner).string(), e.what()));
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

void visitDependencyClosure(
    const std::map<std::string, std::shared_ptr<pkg::IPackage>>& packages,
    std::vector<std::shared_ptr<pkg::IPackage>>& result,
    std::set<std::string>& visited,
    const std::string& name,
    const std::string& packageName
) {
    if (visited.contains(name)) {
        return;
    }

    visited.insert(name);

    if (!packages.contains(name)) {
        throw std::runtime_error("Unknown package: " + name);
    }

    auto& package = packages.at(name);
    for (const auto& dep : package->dependencies()) {
        visitDependencyClosure(packages, result, visited, dep, packageName);
    }

    if (package->name() != packageName) {
        result.push_back(package);
    }
}

void visitBuildDependencyClosure(
    const std::map<std::string, std::shared_ptr<pkg::IPackage>>& packages,
    std::vector<std::shared_ptr<pkg::IPackage>>& result,
    std::set<std::string>& visited,
    const std::string& name,
    const std::string& packageName
) {
    if (visited.contains(name)) {
        return;
    }

    visited.insert(name);

    if (!packages.contains(name)) {
        throw std::runtime_error("Unknown package: " + name);
    }

    auto& package = packages.at(name);
    for (const auto& dep : package->buildDependencies()) {
        visitBuildDependencyClosure(packages, result, visited, dep, packageName);
    }

    for (const auto& dep : package->dependencies()) {
        visitBuildDependencyClosure(packages, result, visited, dep, packageName);
    }

    for (const auto& dep : package->testDependencies()) {
        visitBuildDependencyClosure(packages, result, visited, dep, packageName);
    }

    if (package->name() != packageName) {
        result.push_back(package);
    }
}

}

std::vector<std::shared_ptr<pkg::IPackage>> pkg::dependencyClosure(IWorkspace& workspace, const std::string& packageName) {
    std::vector<std::shared_ptr<IPackage>> result;
    std::set<std::string> visited;

    const auto& packages = workspace.packages();
    visitDependencyClosure(packages, result, visited, packageName, packageName);

    return result;
}

std::vector<std::shared_ptr<pkg::IPackage>> pkg::buildDependencyClosure(IWorkspace& workspace, const std::string& packageName) {
    std::vector<std::shared_ptr<IPackage>> result;
    std::set<std::string> visited;

    const auto& packages = workspace.packages();
    visitBuildDependencyClosure(packages, result, visited, packageName, packageName);

    return result;
}

std::shared_ptr<IWorkspace> IWorkspace::ofRootPath(const fs::path& root) {
    return std::make_shared<WorkspaceImpl>(root);
}
