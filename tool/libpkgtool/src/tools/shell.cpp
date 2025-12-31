#include "pkgtool/build.hpp"
#include "pkgtool/pkgtool.hpp"
#include "src/xml.hpp"

#include "basic.hpp"

#include <vector>

#include "src/exec.hpp"

#include <quill/Frontend.h>

namespace fs = std::filesystem;

namespace {
class ShellBuildTool final : public pkg::BasicBuildTool {
    static inline auto logger() {
        static auto it = quill::Frontend::create_or_get_logger("ShellBuildTool", quill::Frontend::get_logger("root"));
        return it;
    }

    fs::path mConfigureScript;
    fs::path mBuildScript;
    fs::path mInstallScript;
    fs::path mTestScript;

    pkg::ExecuteResult runShellScript(const fs::path& script) {
        if (script.empty()) {
            return pkg::ExecuteResult{0};
        }

        std::vector<std::string> cmd = {
            "/bin/sh", (sourcePath() / script).string()
        };

        auto source = sourcePath().string();

        auto env = environment();

        int result = pkg::execute(logger(), cmd, subprocess::environment{env}, subprocess::cwd{source});

        return pkg::ExecuteResult{result};
    }
public:
    ShellBuildTool(XmlNode node, pkg::IWorkspace& workspace, pkg::IPackage& package)
        : pkg::BasicBuildTool(node, workspace, package)
        , mConfigureScript(node.name() == "configure" ? node.expect("script") : "")
        , mBuildScript(node.name() == "build" ? node.expect("script") : "")
        , mInstallScript(node.name() == "install" ? node.expect("script") : "")
        , mTestScript(node.name() == "test" ? node.expect("script") : "")
    { }

    std::string name() const override {
        return "shell";
    }

    pkg::ExecuteResult configure() override {
        return runShellScript(mConfigureScript);
    }

    pkg::ExecuteResult build() override {
        return runShellScript(mBuildScript);
    }

    pkg::ExecuteResult install() override {
        return runShellScript(mInstallScript);
    }

    pkg::ExecuteResult test() override {
        return runShellScript(mTestScript);
    }
};
}

std::shared_ptr<pkg::ITool> pkg::detail::getShellBuildTool(XmlNode node, IWorkspace& workspace, IPackage& package) {
    return std::make_shared<ShellBuildTool>(node, workspace, package);
}
