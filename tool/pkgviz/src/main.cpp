#include "fmt/format.h"

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/sinks/ConsoleSink.h>
#include <quill/LogMacros.h>
#include <quill/std/FilesystemPath.h>
#include <quill/std/Vector.h>

#include <argparse/argparse.hpp>

#include "pkgtool/state.hpp"
#include "pkgtool/pkgtool.hpp"

#include "defer.hpp"

namespace fs = std::filesystem;

namespace {
quill::Logger* gLogger;
class ArgOptions {
    static constexpr char kConfigKey[] = "--config";
    static constexpr char kProfileKey[] = "--profile";

    static constexpr char kOutputKey[] = "--output";

    static constexpr char kSysrootKey[] = "--sysroot";

    static constexpr char kLogLevelKey[] = "--log-level";

    argparse::ArgumentParser parser;

public:
    ArgOptions()
        : parser{"BezOS package visualization tool"}
    {
        parser.add_argument(kConfigKey)
            .help("Path to the workspace configuration file")
            .default_value(std::string{"workspace.xml"});

        parser.add_argument(kProfileKey)
            .help("Path to profile file")
            .required();

        parser.add_argument(kOutputKey)
            .help("Path to output visualization file")
            .default_value(std::string{"pkgviz.dot"});

        parser.add_argument(kSysrootKey)
            .help("The package to visualize sysroot contributions for")
            .required();

        parser.add_argument(kLogLevelKey)
            .help("Logging level (tracel3, tracel2, tracel1, debug, info, warning, error, critical)")
            .default_value(std::string{"info"});
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

    std::string output() const {
        return parser.get<std::string>(kOutputKey);
    }

    std::string logLevel() const {
        return parser.get<std::string>(kLogLevelKey);
    }

    std::string sysroot() const {
        return parser.get<std::string>(kSysrootKey);
    }
};

void setupLogger() {
    quill::Backend::start();

    quill::ConsoleSinkConfig config;
    config.set_stream("stderr");
    auto console = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("root", config);
    quill::PatternFormatterOptions pattern{"%(time) [%(thread_id)] %(log_level:<6) %(short_source_location:<12) %(message)", "%Y-%m-%dT%H:%M:%S.%QmsZ", quill::Timezone::GmtTime};
    gLogger = quill::Frontend::create_or_get_logger("root", std::move(console), pattern);
    gLogger->set_log_level(quill::LogLevel::Info);
}

void collectDependencyGraph(
    std::ofstream& output,
    pkg::IWorkspace& workspace,
    const std::string& packageName
) {
    for (const auto& depName : pkg::directDependencySet(workspace, packageName, pkg::DependencyScope::ePublicDependency)) {
        output << fmt::format("    \"{}\" -> \"{}\" [color=red];\n", packageName, depName->name());
        collectDependencyGraph(output, workspace, depName->name());
    }
}

int run(int argc, const char** argv) try {
    ArgOptions options;
    options.parse(argc, argv);

    auto levelText = options.logLevel();
    if (levelText != "info" && levelText != "warning" && levelText != "error" &&
        levelText != "debug" && levelText != "critical" &&
        levelText != "tracel1" && levelText != "tracel2" && levelText != "tracel3") {
        LOG_ERROR(gLogger, "Invalid log level '{}'", levelText);
        return 1;
    }

    gLogger->set_log_level(quill::loglevel_from_string(levelText));

    LOG_INFO(gLogger, "Starting pkgviz with config: {}, profile: {}, output: {}, sysroot: {}",
        options.config(), options.profile(), options.output(), options.sysroot());

    fs::path configPath = options.config();

    auto workspace = pkg::IWorkspace::ofRootPath(configPath);

    auto state = pkg::IWorkspaceState::ofSqlite(configPath.parent_path() / "build/workspace.db");

    auto downloadClient = pkg::IDownloadClient::create(configPath.parent_path() / "build/packagecache");

    auto pkgtool = pkg::IPkgTool::create(workspace, state, downloadClient);

    fs::path outpath = fs::absolute(options.output());

    std::ofstream output{outpath};

    output << "digraph pkgviz {\n";
    for (const auto& package : pkg::directDependencySet(*workspace, options.sysroot(), pkg::DependencyScope::ePrivateDependency)) {
        output << fmt::format("    \"{}\" -> \"{}\" [color=blue]\n", options.sysroot(), package->name());
        collectDependencyGraph(output, *workspace, package->name());
    }
    for (const auto& package : pkg::directDependencySet(*workspace, options.sysroot(), pkg::DependencyScope::ePublicDependency)) {
        output << fmt::format("    \"{}\" -> \"{}\" [color=red]\n", options.sysroot(), package->name());
        collectDependencyGraph(output, *workspace, package->name());
    }
    output << "}\n";

    LOG_INFO(gLogger, "Visualization written to {}", outpath);

    return 0;
} catch (const std::exception& ex) {
    LOG_CRITICAL(gLogger, "Fatal error: {}", ex.what());
    return 1;
}
}

int main(int argc, const char** argv) {
    setupLogger();

    defer {
        gLogger->flush_log();
        quill::Backend::stop();
    };

    return run(argc, argv);
}
