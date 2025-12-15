#include "pkgtool/pkgtool.hpp"

#include <argparse/argparse.hpp>

class ArgOptions {
    static constexpr char kConfigKey[] = "--config";
    static constexpr char kProfileKey[] = "--profile";
    static constexpr char kWorkspaceKey[] = "--workspace";

    argparse::ArgumentParser parser;
public:
    ArgOptions()
        : parser{"BezOS package repository manager"}
    {
        parser.add_argument(kConfigKey)
            .help("Path to the workspace configuration file")
            .default_value("workspace.xml");

        parser.add_argument(kProfileKey)
            .help("Path to profile file")
            .required();

        parser.add_argument(kWorkspaceKey)
            .help("Path to vscode workspace file to generate");
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

int main(int argc, const char **argv) {
    ArgOptions options;
    options.parse(argc, argv);

    std::cout << "Config path: " << options.config() << "\n";
    std::cout << "Profile path: " << options.profile() << "\n";
    std::cout << "Workspace path: " << options.workspace() << "\n";

    return 0;
}
