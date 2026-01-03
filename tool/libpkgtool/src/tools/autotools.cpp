#include "pkgtool/build.hpp"
#include "pkgtool/pkgtool.hpp"
#include "src/xml.hpp"
#include "src/exec.hpp"

#include "basic.hpp"

#include <vector>
#include <string>

#include <quill/Frontend.h>
#include <quill/std/Vector.h>

#include <fmt/format.h>

namespace fs = std::filesystem;

namespace {
class AutoToolsBuildTool final : public pkg::BasicBuildTool {
    static inline auto logger() {
        static auto it = quill::Frontend::create_or_get_logger("AutoToolsBuildTool", quill::Frontend::get_logger("root"));
        return it;
    }

    bool mShouldReconfigure;
    fs::path mAutoreconfExecutable;

    pkg::ExecuteResult runCommand(const std::vector<std::string>& args, bool useWorkPath = true) {
        std::vector<std::string> cmd = args;

        auto src = (externalSourcePath().empty() ? sourcePath() : externalSourcePath()).string();

        auto cwd = (!workPath().empty() && useWorkPath) ? workPath().string() : src;
        LOG_INFO(logger(), "Running command in '{}': {}", cwd, cmd);
        int result = pkg::execute(logger(), cmd, subprocess::environment{environment()}, subprocess::cwd{cwd});
        return pkg::ExecuteResult{result};
    }

public:
    AutoToolsBuildTool(XmlNode node, pkg::IWorkspace& workspace, pkg::IPackage& package)
        : pkg::BasicBuildTool(node, workspace, package)
        , mShouldReconfigure(node.property("reconfigure").value_or("false") == "true")
        , mAutoreconfExecutable(pkg::evaluate(node.property("autoreconf").value_or("autoreconf"), workspace, package))
    { }

    std::string name() const override {
        return "autotools";
    }

    pkg::ExecuteResult configure() override {
        if (mShouldReconfigure) {
            std::vector<std::string> cmd = {
                mAutoreconfExecutable.string(), "--install"
            };

            auto result = runCommand(cmd, false);
            if (result.getExitCode() != 0) {
                return result;
            }
        }

        auto src = (externalSourcePath().empty() ? sourcePath() : externalSourcePath()).string();

        std::vector<std::string> cmd = {
            "/bin/sh",
            src + "/configure",
            "--prefix=" + installPrefix().string(),
        };

        for (const auto& [key, value] : options()) {
            std::string val = value;
            if (val.empty()) {
                cmd.push_back("--" + key);
            } else {
                cmd.push_back("--" + key + "=" + val);
            }
        }

        for (const auto& flag : flags()) {
            cmd.push_back(flag);
        }

        return runCommand(cmd);
    }

    pkg::ExecuteResult build() override {
        //
        // Specify -Otarget so the build output isnt interleaved
        //
        std::vector<std::string> cmd = {
            "make", fmt::format("-j{}", std::thread::hardware_concurrency()), "-Otarget"
        };

        return runCommand(cmd);
    }

    pkg::ExecuteResult install() override {
        std::vector<std::string> cmd = {
            "make", "install", "-Otarget"
        };

        return runCommand(cmd);
    }

    pkg::ExecuteResult test() override {
        std::vector<std::string> cmd = {
            "make", "check", "-Otarget"
        };

        return runCommand(cmd);
    }
};
}

std::shared_ptr<pkg::ITool> pkg::detail::getAutoToolsBuildTool(XmlNode node, IWorkspace& workspace, IPackage& package) {
    return std::make_shared<AutoToolsBuildTool>(node, workspace, package);
}
