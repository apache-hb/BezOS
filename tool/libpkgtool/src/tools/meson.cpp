#include "pkgtool/build.hpp"

#include "pkgtool/pkgtool.hpp"
#include "src/exec.hpp"
#include "src/xml.hpp"

#include "basic.hpp"

#include <subprocess.hpp>

namespace {
class MesonBuildTool final : public pkg::BasicBuildTool {
    std::filesystem::path mCrossFile;
    std::filesystem::path mNativeFile;

    std::filesystem::path mBuildPath;
    std::filesystem::path mInstallPrefix;
    std::filesystem::path mSysrootPath;
    std::filesystem::path mSourcePath;

    pkg::ExecuteResult runMesonCommand(const std::vector<std::string>& args) {
        std::vector<std::string> cmd = {
            "meson"
        };

        cmd.insert(cmd.end(), args.begin(), args.end());

        auto source = mSourcePath.string();
        int result = pkg::execute(cmd, subprocess::environment{environment()}, subprocess::cwd{source});

        return pkg::ExecuteResult{result};
    }

public:
    MesonBuildTool(XmlNode node, pkg::IWorkspace& workspace, pkg::IPackage& package)
        : pkg::BasicBuildTool(node)
        , mBuildPath(pkg::packageBuildPath(workspace, package))
        , mInstallPrefix(pkg::packageInstallPath(workspace, package))
        , mSysrootPath(pkg::packageSysrootPath(workspace, package))
        , mSourcePath(package.path())
    { }

    std::string name() const override {
        return "meson";
    }

    pkg::ExecuteResult configure() override {
        std::vector<std::string> cmd = {
            "setup", mBuildPath.string(), mSourcePath.string(),
            "--prefix", mInstallPrefix.string(),
        };

        if (!mCrossFile.empty()) {
            cmd.push_back("--cross-file");
            cmd.push_back(mCrossFile.string());
        }

        if (!mNativeFile.empty()) {
            cmd.push_back("--native-file");
            cmd.push_back(mNativeFile.string());
        }

        for (const auto& [key, value] : options()) {
            cmd.push_back("-D" + key + "=" + value);
        }

        return runMesonCommand(cmd);
    }

    pkg::ExecuteResult build() override {
        std::vector<std::string> cmd = {
            "compile", "-C", mBuildPath.string()
        };

        return runMesonCommand(cmd);
    }

    pkg::ExecuteResult install() override {
        std::vector<std::string> cmd = {
            "install", "-C", mBuildPath.string(), "--skip-subprojects"
        };

        return runMesonCommand(cmd);
    }

    pkg::ExecuteResult test() override {
        std::vector<std::string> cmd = {
            "test", "-C", mBuildPath.string()
        };

        return runMesonCommand(cmd);
    }
};
}

std::shared_ptr<pkg::ITool> pkg::detail::getMesonBuildTool(XmlNode node, IWorkspace& workspace, IPackage& package) {
    return std::make_shared<MesonBuildTool>(node, workspace, package);
}
