#include "pkgtool/build.hpp"
#include "src/xml.hpp"

#include "basic.hpp"

namespace {
class MakeBuildTool final : public pkg::BasicBuildTool {
public:
    MakeBuildTool(XmlNode node)
        : pkg::BasicBuildTool(node)
    { }

    std::string name() const override {
        return "make";
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

std::shared_ptr<pkg::ITool> pkg::detail::getMakeBuildTool(XmlNode node) {
    return std::make_shared<MakeBuildTool>(node);
}
