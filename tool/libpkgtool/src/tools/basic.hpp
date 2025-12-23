#pragma once

#include "pkgtool/build.hpp"

#include "src/xml.hpp"

#include <map>

namespace pkg {
class IWorkspace;
class IPackage;

class BasicBuildTool : public ITool {
    std::map<std::string, std::string> mEnvironment;
    std::map<std::string, std::string> mOptions;

protected:
    const std::map<std::string, std::string>& environment() const;

    const std::map<std::string, std::string>& options() const;

    BasicBuildTool(XmlNode node);
};

namespace detail {
std::shared_ptr<ITool> getMesonBuildTool(XmlNode node, IWorkspace& workspace, IPackage& package);
std::shared_ptr<ITool> getCMakeBuildTool(XmlNode node);
std::shared_ptr<ITool> getMakeBuildTool(XmlNode node);
std::shared_ptr<ITool> getAutoToolsBuildTool(XmlNode node);
std::shared_ptr<ITool> getShellBuildTool(XmlNode node);
}

std::shared_ptr<ITool> getTool(XmlNode node, IWorkspace& workspace, IPackage& package);
}
