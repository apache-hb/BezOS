#pragma once

#include "pkgtool/build.hpp"

#include "src/xml.hpp"

#include <map>

namespace pkg {
class IWorkspace;
class IPackage;

class BasicBuildTool : public ITool {
    std::map<std::string, std::string> mEnvironment;
    std::map<std::string, std::string> mOptions;
    std::vector<std::string> mFlags;

    std::filesystem::path mBuildPath;
    std::filesystem::path mInstallPrefix;
    std::filesystem::path mSysrootPath;
    std::filesystem::path mSourcePath;
    std::filesystem::path mCachePath;

    std::filesystem::path mWorkPath;
    std::filesystem::path mExternalSourcePath;

protected:
    const std::map<std::string, std::string>& environment() const;

    const std::map<std::string, std::string>& options() const;
    const std::vector<std::string>& flags() const;

    const std::filesystem::path& buildPath() const;
    const std::filesystem::path& installPrefix() const;
    const std::filesystem::path& sysrootPath() const;
    const std::filesystem::path& sourcePath() const;
    const std::filesystem::path& cachePath() const;

    const std::filesystem::path& workPath() const {
        return mWorkPath;
    }

    const std::filesystem::path& externalSourcePath() const {
        return mExternalSourcePath;
    }

    BasicBuildTool(XmlNode node, IWorkspace& workspace, IPackage& package);
};

namespace detail {
std::shared_ptr<ITool> getMesonBuildTool(XmlNode node, IWorkspace& workspace, IPackage& package);
std::shared_ptr<ITool> getCMakeBuildTool(XmlNode node, IWorkspace& workspace, IPackage& package);
std::shared_ptr<ITool> getMakeBuildTool(XmlNode node, IWorkspace& workspace, IPackage& package);
std::shared_ptr<ITool> getAutoToolsBuildTool(XmlNode node, IWorkspace& workspace, IPackage& package);
std::shared_ptr<ITool> getShellBuildTool(XmlNode node, IWorkspace& workspace, IPackage& package);
std::shared_ptr<ITool> getCustomBuildTool(XmlNode node, IWorkspace& workspace, IPackage& package);
}

std::shared_ptr<ITool> getTool(XmlNode node, IWorkspace& workspace, IPackage& package);
}
