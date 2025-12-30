#include "pkgtool/build.hpp"
#include "pkgtool/pkgtool.hpp"

#include "src/xml.hpp"
#include "src/exec.hpp"

#include "basic.hpp"

#include <quill/Frontend.h>

namespace {
class CMakeBuildTool final : public pkg::BasicBuildTool {
    static inline auto logger() {
        static auto it = quill::Frontend::create_or_get_logger("CMakeBuildTool", quill::Frontend::get_logger("root"));
        return it;
    }

    std::filesystem::path mToolchainFile;

    pkg::ExecuteResult runCMakeCommand(const std::vector<std::string>& args) {
        std::vector<std::string> cmd = {
            "cmake"
        };

        cmd.insert(cmd.end(), args.begin(), args.end());

        auto source = sourcePath().string();
        int result = pkg::execute(logger(), cmd, subprocess::environment{environment()}, subprocess::cwd{source});

        return pkg::ExecuteResult{result};
    }

public:
    CMakeBuildTool(XmlNode node, pkg::IWorkspace& workspace, pkg::IPackage& package)
        : pkg::BasicBuildTool(node, workspace, package)
    { }

    std::string name() const override {
        return "cmake";
    }

    pkg::ExecuteResult configure() override {
       std::vector<std::string> cmd = {
            "-B", buildPath().string(),
            "-S", sourcePath().string(),
            "-DCMAKE_INSTALL_PREFIX=" + installPrefix().string(),
            "-DCMAKE_BUILD_TYPE=MinSizeRel",
            "-DCMAKE_SYSROOT=" + sysrootPath().string(),
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
            "-G", "Ninja"
        };

        for (const auto& [key, value] : options()) {
            cmd.push_back("-D" + key + "=" + value);
        }

        if (!mToolchainFile.empty()) {
            cmd.push_back("-DCMAKE_TOOLCHAIN_FILE=" + mToolchainFile.string());
        }

        return runCMakeCommand(cmd);
    }

    pkg::ExecuteResult build() override {
        std::vector<std::string> cmd = {
            "--build", buildPath().string(),
        };

        return runCMakeCommand(cmd);
    }

    pkg::ExecuteResult install() override {
        std::vector<std::string> cmd = {
            "--build", buildPath().string(),
            "--target", "install",
        };

        return runCMakeCommand(cmd);
    }

    pkg::ExecuteResult test() override {
        std::vector<std::string> cmd = {
            "--build", buildPath().string(),
            "--target", "test",
        };

        return runCMakeCommand(cmd);
    }
};
}

std::shared_ptr<pkg::ITool> pkg::detail::getCMakeBuildTool(XmlNode node, IWorkspace& workspace, IPackage& package) {
    return std::make_shared<CMakeBuildTool>(node, workspace, package);
}
