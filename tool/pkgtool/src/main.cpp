#include "pkgtool/pkgtool.hpp"

#include "defer.hpp"

#include <argparse/argparse.hpp>
#include <libxml/parser.h>

namespace fs = std::filesystem;

class ArgOptions {
    static constexpr char kConfigKey[] = "--config";
    static constexpr char kProfileKey[] = "--profile";
    static constexpr char kWorkspaceKey[] = "--vsc-workspace";

    argparse::ArgumentParser parser;
public:
    ArgOptions()
        : parser{"BezOS package repository manager"}
    {
        parser.add_argument(kConfigKey)
            .help("Path to the workspace configuration file")
            .default_value(std::string{"workspace.xml"});

        parser.add_argument(kProfileKey)
            .help("Path to profile file")
            .required();

        parser.add_argument(kWorkspaceKey)
            .help("Path to vscode workspace file to generate")
            .default_value(std::string{"workspace.code-workspace"})
            .implicit_value(std::string{"workspace.code-workspace"});
    }

    void parse(int argc, const char** argv) {
        parser.parse_args(argc, argv);
    }

    std::string config() const {
        return parser.get<std::string>(kConfigKey);
    }

    std::string profile() const {
        return parser.get<std::string>(kProfileKey);
    }

    std::string workspace() const {
        return parser.get<std::string>(kWorkspaceKey);
    }
};

int main(int argc, const char **argv) try {
    LIBXML_TEST_VERSION;
    defer { xmlCleanupParser(); };

    ArgOptions options;
    options.parse(argc, argv);

    std::cout << "Config path: " << options.config() << "\n";
    std::cout << "Profile path: " << options.profile() << "\n";
    std::cout << "Workspace path: " << options.workspace() << "\n";

    fs::path configPath = options.config();

    auto workspace = pkg::IWorkspace::ofRootPath(configPath);
    auto packages = workspace->packages();
    for (const auto& [name, package] : packages) {
        std::cout << "Package: " << name << " at " << package->path() << "\n";
    }

    return 0;
} catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << "\n";
    return 1;
}
