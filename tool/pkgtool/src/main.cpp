#include "pkgtool/pkgtool.hpp"

#include <argparse/argparse.hpp>

class ArgOptions {
    static constexpr std::string kConfigKey = "--config";
    static constexpr std::string kTargetKey = "--target";
    static constexpr std::string kWorkspaceKey = "--workspace";

    argparse::ArgumentParser parser;
public:
    ArgOptions()
        : parser{"BezOS package repository manager"}
    {
        parser.add_argument(kConfigKey)
            .help("Path to the configuration file")
            .default_value("repo.xml");

        parser.add_argument(kTargetKey)
            .help("Path to target file")
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

    std::string target() const {
        return parser.get<std::string>(kTargetKey);
    }

    std::string workspace() const {
        return parser.get<std::string>(kWorkspaceKey);
    }
};

int main() {

}
