#include "pkgtool/build.hpp"
#include "src/xml.hpp"

#include "basic.hpp"

namespace {
class CMakeBuildTool final : public pkg::BasicBuildTool {
    std::filesystem::path mToolchainFile;
public:
    CMakeBuildTool(XmlNode node)
        : pkg::BasicBuildTool(node)
    { }

    std::string name() const override {
        return "cmake";
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

std::shared_ptr<pkg::ITool> pkg::detail::getCMakeBuildTool(XmlNode node) {
    return std::make_shared<CMakeBuildTool>(node);
}
