#include "pkgtool/build.hpp"
#include "src/xml.hpp"
#include "src/exec.hpp"

#include "basic.hpp"

#include <vector>
#include <string>

#include <quill/Frontend.h>

namespace {
class AutoToolsBuildTool final : public pkg::BasicBuildTool {
    static inline auto logger() {
        static auto it = quill::Frontend::create_or_get_logger("AutoToolsBuildTool", quill::Frontend::get_logger("root"));
        return it;
    }

    bool mShouldReconfigure;

    pkg::ExecuteResult runCommand(const std::vector<std::string>& args) {
        std::vector<std::string> cmd = args;

        auto cwd = sourcePath().string();
        int result = pkg::execute(logger(), cmd, subprocess::environment{environment()}, subprocess::cwd{cwd});
        return pkg::ExecuteResult{result};
    }

public:
    AutoToolsBuildTool(XmlNode node, pkg::IWorkspace& workspace, pkg::IPackage& package)
        : pkg::BasicBuildTool(node, workspace, package)
        , mShouldReconfigure(node.property("autoreconf").value_or("false") == "true")
    { }

    std::string name() const override {
        return "autotools";
    }

    pkg::ExecuteResult configure() override {
        if (mShouldReconfigure) {
            std::vector<std::string> cmd = {
                "autoreconf", "--install"
            };

            auto result = runCommand(cmd);
            if (result.getExitCode() != 0) {
                return result;
            }
        }

        std::vector<std::string> cmd = {
            "/bin/sh",
            sourcePath().string() + "/configure",
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

        return runCommand(cmd);
    }

    pkg::ExecuteResult build() override {
        //
        // Specify -Otarget so the build output isnt interleaved
        //
        std::vector<std::string> cmd = {
            "make", std::format("-j{}", std::thread::hardware_concurrency()), "-Otarget"
        };

        return runCommand(cmd);
    }

    pkg::ExecuteResult install() override {
        return pkg::ExecuteResult{};
    }

    pkg::ExecuteResult test() override {
        return pkg::ExecuteResult{};
    }
};
}

std::shared_ptr<pkg::ITool> pkg::detail::getAutoToolsBuildTool(XmlNode node, IWorkspace& workspace, IPackage& package) {
    return std::make_shared<AutoToolsBuildTool>(node, workspace, package);
}
