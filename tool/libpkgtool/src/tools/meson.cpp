#include "pkgtool/build.hpp"

#include "pkgtool/pkgtool.hpp"
#include "src/exec.hpp"
#include "src/xml.hpp"

#include "basic.hpp"

#include <quill/Frontend.h>

namespace {
class MesonBuildTool final : public pkg::BasicBuildTool {
    static inline auto logger() {
        static auto it = quill::Frontend::create_or_get_logger("MesonBuildTool", quill::Frontend::get_logger("root"));
        return it;
    }

    std::filesystem::path mCrossFile;
    std::filesystem::path mNativeFile;

    pkg::ExecuteResult runMesonCommand(const std::vector<std::string>& args) {
        std::vector<std::string> cmd = {
            "meson"
        };

        cmd.insert(cmd.end(), args.begin(), args.end());

        auto source = sourcePath().string();
        int result = pkg::execute(logger(), cmd, subprocess::environment{environment()}, subprocess::cwd{source});

        return pkg::ExecuteResult{result};
    }

public:
    MesonBuildTool(XmlNode node, pkg::IWorkspace& workspace, pkg::IPackage& package)
        : pkg::BasicBuildTool(node, workspace, package)
    { }

    std::string name() const override {
        return "meson";
    }

    pkg::ExecuteResult configure() override {
        std::vector<std::string> cmd = {
            "setup", buildPath().string(), sourcePath().string(),
            "--prefix", installPrefix().string(),
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
            "compile", "-C", buildPath().string()
        };

        return runMesonCommand(cmd);
    }

    pkg::ExecuteResult install() override {
        std::vector<std::string> cmd = {
            "install", "-C", buildPath().string(), "--skip-subprojects"
        };

        return runMesonCommand(cmd);
    }

    pkg::ExecuteResult test() override {
        std::vector<std::string> cmd = {
            "test", "-C", buildPath().string()
        };

        return runMesonCommand(cmd);
    }
};
}

std::shared_ptr<pkg::ITool> pkg::detail::getMesonBuildTool(XmlNode node, IWorkspace& workspace, IPackage& package) {
    return std::make_shared<MesonBuildTool>(node, workspace, package);
}
