#include "pkgtool/build.hpp"
#include "src/xml.hpp"
#include "src/exec.hpp"

#include "basic.hpp"

#include <quill/Frontend.h>

namespace {
class CustomBuildTool final : public pkg::BasicBuildTool {
    static inline auto logger() {
        static auto it = quill::Frontend::create_or_get_logger("CustomBuildTool", quill::Frontend::get_logger("root"));
        return it;
    }

    std::string mTool;

    pkg::ExecuteResult runCommand(const std::string& phase) {
        std::vector<std::string> cmd = {
            (sysrootPath() / "bin" / mTool).string(), phase
        };

        auto source = sourcePath().string();

        auto env = environment();
        int result = pkg::execute(logger(), cmd, subprocess::environment{env}, subprocess::cwd{source});

        return pkg::ExecuteResult{result};
    }
public:
    CustomBuildTool(XmlNode node, pkg::IWorkspace& workspace, pkg::IPackage& package)
        : pkg::BasicBuildTool(node, workspace, package)
        , mTool(node.expect("tool"))
    { }

    std::string name() const override {
        return "custom";
    }

    pkg::ExecuteResult configure() override {
        return runCommand("configure");
    }

    pkg::ExecuteResult build() override {
        return runCommand("build");
    }

    pkg::ExecuteResult install() override {
        return runCommand("install");
    }

    pkg::ExecuteResult test() override {
        return runCommand("test");
    }
};
}

std::shared_ptr<pkg::ITool> pkg::detail::getCustomBuildTool(XmlNode node, IWorkspace& workspace, IPackage& package) {
    return std::make_shared<CustomBuildTool>(node, workspace, package);
}
