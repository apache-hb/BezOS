#include "pkgtool/build.hpp"
#include "src/xml.hpp"

#include "basic.hpp"

namespace {
class ShellBuildTool final : public pkg::BasicBuildTool {
    std::string mConfigureScript;
    std::string mBuildScript;
    std::string mInstallScript;
public:
    ShellBuildTool(XmlNode node)
        : pkg::BasicBuildTool(node)
    {

    }

    std::string name() const override {
        return "shell";
    }

    pkg::ExecuteResult configure() override {
        return pkg::ExecuteResult{};
    }

    pkg::ExecuteResult build() override {
        return pkg::ExecuteResult{};
    }

    pkg::ExecuteResult install() override {
        return pkg::ExecuteResult{};
    }

    pkg::ExecuteResult test() override {
        return pkg::ExecuteResult{};
    }
};
}

std::shared_ptr<pkg::ITool> pkg::detail::getShellBuildTool(XmlNode node) {
    return std::make_shared<ShellBuildTool>(node);
}
