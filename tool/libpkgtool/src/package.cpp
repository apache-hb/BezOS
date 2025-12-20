#include "pkgtool/pkgtool.hpp"
#include "src/xml.hpp"

#include <vector>

using pkg::IPackage;

namespace fs = std::filesystem;

namespace {

class PackageName {
    std::string mName;
    std::string mVersion;

public:
    PackageName(const std::string& name, const std::string& version)
        : mName(name)
        , mVersion(version)
    { }

    const std::string& name() const {
        return mName;
    }

    const std::string& version() const {
        return mVersion;
    }

    static PackageName ofXmlNode(const XmlNode& node) {
        auto name = node.expect("name");
        auto version = node.property("version").value_or("0.0.0");

        return PackageName(name, version);
    }
};

class PackageImpl final : public IPackage {
    fs::path mFolder;
    std::vector<PackageName> mDependencies;
    std::vector<PackageName> mBuildDependencies;
    std::vector<PackageName> mTestDependencies;
    std::string mBuildTool;

    std::string mName;
    std::string mVersion;
public:
    PackageImpl(const fs::path& folder)
        : mFolder(folder)
    {
        auto pkginfo = mFolder / "pkg.xml";
        if (!fs::exists(pkginfo)) {
            throw std::runtime_error("Package folder " + folder.string() + " does not contain pkg.xml");
        }

        auto document = XmlDocument::parse(pkginfo);

        XmlNode root = document.root();
        if (root.name() != "package") {
            throw std::runtime_error(std::format("ERROR [{}:{}]: Invalid root element <{}> in {}, expected <package>", root.path(), root.line(), root.name(), pkginfo.string()));
        }

        mName = root.expect("name");
        mVersion = root.property("version").value_or("0.0.0");

        for (auto child : root.children()) {
            auto name = child.name();
            if (name == "text" || name == "comment") {
                continue;
            }

            if (name == "dependency") {
                mDependencies.emplace_back(PackageName::ofXmlNode(child));
            } else if (name == "build-dependency") {
                mBuildDependencies.emplace_back(PackageName::ofXmlNode(child));
            } else if (name == "test-dependency") {
                mTestDependencies.emplace_back(PackageName::ofXmlNode(child));
            } else if (name == "build") {
                mBuildTool = child.expect("with");
            } else {
                throw std::runtime_error(std::format("ERROR [{}:{}]: Unknown element <{}> in {}", child.path(), child.line(), name, pkginfo.string()));
            }
        }

        if (mBuildTool.empty()) {
            throw std::runtime_error(std::format("ERROR [{}:{}]: Missing <build> element in {}", root.path(), root.line(), pkginfo.string()));
        }
    }

    std::string name() const override {
        return mName;
    }

    std::string buildTool() const override {
        return mBuildTool;
    }

    std::filesystem::path path() const override {
        return mFolder;
    }

    std::vector<std::string> buildDependencies() const override {
        std::vector<std::string> result;
        for (const auto& dep : mBuildDependencies) {
            result.push_back(dep.name());
        }
        return result;
    }

    std::vector<std::string> testDependencies() const override {
        std::vector<std::string> result;
        for (const auto& dep : mTestDependencies) {
            result.push_back(dep.name());
        }
        return result;
    }

    std::vector<std::string> dependencies() const override {
        std::vector<std::string> result;
        for (const auto& dep : mDependencies) {
            result.push_back(dep.name());
        }
        return result;
    }
};

std::filesystem::path baseBuildPath(pkg::IWorkspace& workspace) {
    return workspace.path() / "build" / "env";
}
}

std::filesystem::path pkg::packageBuildPath(IWorkspace& workspace, IPackage& package) {
    return baseBuildPath(workspace) / package.name() / "target/build";
}

std::filesystem::path pkg::packageSysrootPath(IWorkspace& workspace, IPackage& package) {
    return baseBuildPath(workspace) / package.name() / "target/sysroot";
}

std::filesystem::path pkg::packageInstallPath(IWorkspace& workspace, IPackage& package) {
    return baseBuildPath(workspace) / package.name() / "target/install";
}

std::filesystem::path pkg::packagePrivatePath(IWorkspace& workspace, IPackage& package) {
    return baseBuildPath(workspace) / package.name() / "target/internal";
}

void pkg::setupPackageEnvironment(IWorkspace& workspace, IPackage& package) {
    auto sysroot = fs::absolute(pkg::packageSysrootPath(workspace, package));
    auto installdir = fs::absolute(pkg::packageInstallPath(workspace, package));
    auto internaldir = fs::absolute(pkg::packagePrivatePath(workspace, package));
    auto workdir = internaldir / "work";

    fs::create_directories(sysroot);
    fs::create_directories(installdir);
    fs::create_directories(internaldir);
    fs::create_directories(workdir);
}

std::shared_ptr<IPackage> IPackage::of(const std::filesystem::path& folder) {
    return std::make_shared<PackageImpl>(folder);
}
