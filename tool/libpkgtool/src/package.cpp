#include "pkgtool/pkgtool.hpp"
#include "src/tools/basic.hpp"
#include "src/xml.hpp"

#include <vector>

#include <quill/Logger.h>
#include <quill/Frontend.h>

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
    static inline auto logger() {
        static auto it = quill::Frontend::create_or_get_logger("PackageImpl", quill::Frontend::get_logger("root"));
        return it;
    }

    fs::path mFolder;

    std::vector<PackageName> mDependencies;
    std::vector<PackageName> mBuildDependencies;
    std::vector<PackageName> mTestDependencies;

    std::shared_ptr<pkg::ITool> mConfigureTool;
    std::shared_ptr<pkg::ITool> mBuildTool;
    std::shared_ptr<pkg::ITool> mInstallTool;
    std::shared_ptr<pkg::ITool> mTestTool;

    std::vector<pkg::DownloadInfo> mSources;

    std::string mName;
    std::string mVersion;
public:
    PackageImpl(const fs::path& folder, pkg::IWorkspace& workspace)
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

        for (auto child : root.elements()) {
            auto name = child.name();
            if (name == "download") {
                auto file = child.expect("file");
                auto url = child.expect("url");
                auto sha256 = child.property("sha256").value_or("");
                auto format = child.property("archive").value_or("");
                auto git = child.property("git").value_or("");
                auto branch = child.property("branch").value_or("");
                auto commit = child.property("commit").value_or("");
                bool trimRootFolder = child.property("trim-root-folder").value_or("false") == "true";

                std::vector<fs::path> patches;
                for (auto patchNode : child.children()) {
                    if (patchNode.name() != "patch") {
                        continue;
                    }

                    auto patchPath = patchNode.expect("file");
                    patches.push_back(mFolder / patchPath);
                }

                pkg::DownloadInfo info {
                    .url = url,
                    .name = file,
                    .sha256Hash = sha256,
                    .format = format,
                    .trimRootFolder = trimRootFolder,
                    .git = git,
                    .branch = branch,
                    .commit = commit,
                    .patches = patches,
                };
                mSources.push_back(info);
            } else if (name == "dependency") {
                mDependencies.emplace_back(PackageName::ofXmlNode(child));
            } else if (name == "build-dependency") {
                mBuildDependencies.emplace_back(PackageName::ofXmlNode(child));
            } else if (name == "test-dependency") {
                mTestDependencies.emplace_back(PackageName::ofXmlNode(child));
            } else if (name == "configure") {
                if (mConfigureTool != nullptr) {
                    throw std::runtime_error("Package " + mName + " has multiple configure tools defined");
                }

                mConfigureTool = pkg::getTool(child, workspace, *this);
            } else if (name == "build") {
                if (mBuildTool != nullptr) {
                    throw std::runtime_error("Package " + mName + " has multiple build tools defined");
                }

                mBuildTool = pkg::getTool(child, workspace, *this);
            } else if (name == "install") {
                if (mInstallTool != nullptr) {
                    throw std::runtime_error("Package " + mName + " has multiple install tools defined");
                }

                mInstallTool = pkg::getTool(child, workspace, *this);
            } else {
                throw std::runtime_error(std::format("ERROR {}: Unknown element <{}> in {}", locationToString(child), name, pkginfo.string()));
            }
        }

        if (mConfigureTool == nullptr) {
            if (mBuildTool != nullptr) {
                mConfigureTool = mBuildTool;
            } else if (mInstallTool != nullptr) {
                mConfigureTool = mInstallTool;
            }
        }

        if (mBuildTool == nullptr) {
            mBuildTool = mConfigureTool;
        }

        if (mInstallTool == nullptr) {
            mInstallTool = mBuildTool;
        }

        if (mConfigureTool == nullptr || mBuildTool == nullptr || mInstallTool == nullptr) {
            throw std::runtime_error("Package " + mName + " is missing build tool definitions");
        }
    }

    std::string name() const override {
        return mName;
    }

    std::shared_ptr<pkg::ITool> configureTool() const override {
        return mConfigureTool;
    }

    std::shared_ptr<pkg::ITool> buildTool() const override {
        return mBuildTool;
    }

    std::shared_ptr<pkg::ITool> installTool() const override {
        return mInstallTool;
    }

    std::shared_ptr<pkg::ITool> testTool() const override {
        return mTestTool;
    }

    std::filesystem::path path() const override {
        return mFolder;
    }

    std::vector<pkg::DownloadInfo> sources() const override {
        return std::vector<pkg::DownloadInfo>{};
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
    return fs::absolute(workspace.path() / "build" / "env");
}

void replaceAll(std::string& str, std::string_view from, std::string_view to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}
}

std::filesystem::path pkg::workspaceCachePath(IWorkspace& workspace) {
    return baseBuildPath(workspace) / "packagecache";
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

std::string pkg::evaluate(const std::string& text, IWorkspace& workspace) {
    std::string result = text;
    replaceAll(result, "${workspace.path}", workspace.path().string());
    return result;
}

std::string pkg::evaluate(const std::string& text, IWorkspace& workspace, IPackage& package) {
    std::string result = evaluate(text, workspace);
    replaceAll(result, "${package.name}", package.name());
    replaceAll(result, "${package.path}", package.path().string());
    replaceAll(result, "${package.build}", pkg::packageBuildPath(workspace, package).string());
    replaceAll(result, "${package.sysroot}", pkg::packageSysrootPath(workspace, package).string());
    return result;
}

void pkg::setupWorkspaceLayout(IWorkspace& workspace) {
    fs::create_directories(baseBuildPath(workspace));
}

void pkg::setupPackageBuildLayout(IWorkspace& workspace, IPackage& package) {
    auto sysroot = pkg::packageSysrootPath(workspace, package);
    auto installdir = pkg::packageInstallPath(workspace, package);
    auto internaldir = pkg::packagePrivatePath(workspace, package);
    auto workdir = internaldir / "work";

    fs::create_directories(sysroot);
    fs::create_directories(installdir);
    fs::create_directories(internaldir);
    fs::create_directories(workdir);
}

std::shared_ptr<IPackage> IPackage::of(const std::filesystem::path& folder, pkg::IWorkspace& workspace) {
    return std::make_shared<PackageImpl>(folder, workspace);
}
