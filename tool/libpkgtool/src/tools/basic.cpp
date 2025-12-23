#include "basic.hpp"

#include <print>

const std::map<std::string, std::string>& pkg::BasicBuildTool::environment() const {
    return mEnvironment;
}

const std::map<std::string, std::string>& pkg::BasicBuildTool::options() const {
    return mOptions;
}

pkg::BasicBuildTool::BasicBuildTool(XmlNode node) {
    for (const auto& child : node.children()) {
        if (child.name() == "text" || child.name() == "comment") {
            continue;
        }

        if (child.name() == "env") {
            for (const auto& [key, value] : child.properties()) {
                if (mEnvironment.contains(key)) {
                    std::println("Warning {} Duplicate environment variable {}", locationToString(child), key);
                }
                mEnvironment.emplace(key, value);
            }
        } else if (child.name() == "option") {
            for (const auto& [key, value] : child.properties()) {
                if (mOptions.contains(key)) {
                    std::println("Warning {} Duplicate option {}", locationToString(child), key);
                }
                mOptions.emplace(key, value);
            }
        }
    }
}

std::shared_ptr<pkg::ITool> pkg::getTool(XmlNode node, IWorkspace& workspace, IPackage& package) {
    const auto name = node.expect("with");

    if (name == "meson") {
        return detail::getMesonBuildTool(node, workspace, package);
    } else if (name == "cmake") {
        return detail::getCMakeBuildTool(node);
    } else if (name == "make") {
        return detail::getMakeBuildTool(node);
    } else if (name == "autotools") {
        return detail::getAutoToolsBuildTool(node);
    } else if (name == "shell") {
        return detail::getShellBuildTool(node);
    }

    throw std::runtime_error(std::format("ERROR {}: Unknown build tool '{}'", locationToString(node), name));
}
