#include "pkgtool/pkgtool.hpp"

#include "defer.hpp"

#include <argparse/argparse.hpp>
#include <libxml/parser.h>

namespace fs = std::filesystem;

class ArgOptions {
    static constexpr char kConfigKey[] = "--config";
    static constexpr char kProfileKey[] = "--profile";
    static constexpr char kWorkspaceKey[] = "--vsc-workspace";

    static constexpr char kFetchKey[] = "--fetch";
    static constexpr char kConfigureKey[] = "--configure";
    static constexpr char kBuildKey[] = "--build";
    static constexpr char kInstallKey[] = "--install";

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

        parser.add_argument(kFetchKey)
            .help("List of packages to fetch or update")
            .append()
            .nargs(argparse::nargs_pattern::any);

        parser.add_argument(kConfigureKey)
            .help("List of packages to configure or reconfigure")
            .append()
            .nargs(argparse::nargs_pattern::any);

        parser.add_argument(kBuildKey)
            .help("List of packages to build or rebuild")
            .append()
            .nargs(argparse::nargs_pattern::any);

        parser.add_argument(kInstallKey)
            .help("List of packages to install or reinstall")
            .append()
            .nargs(argparse::nargs_pattern::any);
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

    std::vector<std::string> fetchPackages() const {
        return parser.get<std::vector<std::string>>(kFetchKey);
    }

    std::vector<std::string> configurePackages() const {
        return parser.get<std::vector<std::string>>(kConfigureKey);
    }

    std::vector<std::string> buildPackages() const {
        return parser.get<std::vector<std::string>>(kBuildKey);
    }

    std::vector<std::string> installPackages() const {
        return parser.get<std::vector<std::string>>(kInstallKey);
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
        pkg::setupPackageEnvironment(*workspace, *package);
    }

    auto closure = pkg::dependencyClosure(*workspace, "image");
    for (const auto& package : closure) {
        std::cout << " - " << package->name() << "\n";
    }

    auto pkgtool = pkg::IPkgTool::create(workspace);
    pkgtool->createPackageEnvironment("image");

    return 0;
} catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << "\n";
    return 1;
}
