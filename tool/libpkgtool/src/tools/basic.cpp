#include "basic.hpp"
#include "pkgtool/pkgtool.hpp"

#include <print>

const std::map<std::string, std::string>& pkg::BasicBuildTool::environment() const {
    return mEnvironment;
}

const std::map<std::string, std::string>& pkg::BasicBuildTool::options() const {
    return mOptions;
}

const std::filesystem::path& pkg::BasicBuildTool::buildPath() const {
    return mBuildPath;
}

const std::filesystem::path& pkg::BasicBuildTool::installPrefix() const {
    return mInstallPrefix;
}

const std::filesystem::path& pkg::BasicBuildTool::sysrootPath() const {
    return mSysrootPath;
}

const std::filesystem::path& pkg::BasicBuildTool::sourcePath() const {
    return mSourcePath;
}

pkg::BasicBuildTool::BasicBuildTool(XmlNode node, IWorkspace& workspace, IPackage& package)
    : mBuildPath(pkg::packageBuildPath(workspace, package))
    , mInstallPrefix(pkg::packageInstallPath(workspace, package))
    , mSysrootPath(pkg::packageSysrootPath(workspace, package))
    , mSourcePath(package.path())
{
    mEnvironment.emplace("PKGTOOL_PREFIX", mInstallPrefix.string());
    mEnvironment.emplace("PKGTOOL_SYSROOT", mSysrootPath.string());
    mEnvironment.emplace("PKGTOOL_BUILDDIR", mBuildPath.string());
    mEnvironment.emplace("PKGTOOL_SOURCEDIR", mSourcePath.string());

    for (const auto& child : node.elements()) {
        if (child.name() == "env") {
            for (const auto& [key, value] : child.properties()) {
                if (mEnvironment.contains(key)) {
                    std::println("Warning {} Duplicate environment variable {}", locationToString(child), key);
                }
                mEnvironment.emplace(key, value);
            }
        } else if (child.name() == "options") {
            for (const auto& [key, value] : child.properties()) {
                if (mOptions.contains(key)) {
                    std::println("Warning {} Duplicate option {}", locationToString(child), key);
                }
                mOptions.emplace(key, value);
            }
        }
    }
}

std::shared_ptr<pkg::ITool> pkg::getTool(XmlNode node, IWorkspace& workspace, IPackage& package) {
    const auto name = node.expect("with");

    if (name == "meson") {
        return detail::getMesonBuildTool(node, workspace, package);
    } else if (name == "cmake") {
        return detail::getCMakeBuildTool(node, workspace, package);
    } else if (name == "make") {
        return detail::getMakeBuildTool(node, workspace, package);
    } else if (name == "autotools") {
        return detail::getAutoToolsBuildTool(node, workspace, package);
    } else if (name == "shell") {
        return detail::getShellBuildTool(node, workspace, package);
    } else if (name == "custom") {
        return detail::getCustomBuildTool(node, workspace, package);
    }

    throw std::runtime_error(std::format("ERROR {}: Unknown build tool '{}'", locationToString(node), name));
}
