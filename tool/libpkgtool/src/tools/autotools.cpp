#include "pkgtool/build.hpp"
#include "src/xml.hpp"

#include "basic.hpp"

namespace {
class AutoToolsBuildTool final : public pkg::BasicBuildTool {
public:
    AutoToolsBuildTool(XmlNode node)
        : pkg::BasicBuildTool(node)
    { }

    std::string name() const override {
        return "autotools";
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

std::shared_ptr<pkg::ITool> pkg::detail::getAutoToolsBuildTool(XmlNode node) {
    return std::make_shared<AutoToolsBuildTool>(node);
}
