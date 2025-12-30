#include "pkgtool/pkgtool.hpp"

#include "defer.hpp"
#include "pkgtool/state.hpp"

#include <argparse/argparse.hpp>
#include <libxml/parser.h>

#include <openssl/crypto.h>
#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/sinks/ConsoleSink.h>
#include <quill/LogMacros.h>
#include <quill/std/FilesystemPath.h>

namespace fs = std::filesystem;

namespace {
quill::Logger* gLogger;

class ArgOptions {
    static constexpr char kConfigKey[] = "--config";
    static constexpr char kProfileKey[] = "--profile";
    static constexpr char kWorkspaceKey[] = "--vsc-workspace";

    static constexpr char kFetchKey[] = "--fetch";
    static constexpr char kConfigureKey[] = "--configure";
    static constexpr char kBuildKey[] = "--build";
    static constexpr char kInstallKey[] = "--install";

    static constexpr char kLogLevelKey[] = "--log-level";

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

        parser.add_argument(kLogLevelKey)
            .help("Set the logging level (tracel3, tracel2, tracel1, debug, info, warning, error, critical)")
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

    std::string logLevel() const {
        return parser.get<std::string>(kLogLevelKey);
    }
};

void setupLogger() {
    quill::Backend::start();

    quill::ConsoleSinkConfig config;
    config.set_stream("stderr");
    auto console = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("root", config);
    quill::PatternFormatterOptions pattern{"%(time) [%(thread_id)] %(short_source_location:<12) %(log_level:<6) %(message)", "%Y-%m-%dT%H:%M:%S.%QmsZ", quill::Timezone::GmtTime};
    gLogger = quill::Frontend::create_or_get_logger("root", std::move(console), pattern);
    gLogger->set_log_level(quill::LogLevel::Info);
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

    LOG_INFO(gLogger, "Starting pkgtool with config: {}, profile: {}, workspace: {}",
        options.config(), options.profile(), options.workspace());

    fs::path configPath = options.config();

    auto workspace = pkg::IWorkspace::ofRootPath(configPath);
    auto packages = workspace->packages();
    for (const auto& [name, package] : packages) {
        LOG_INFO(gLogger, "Found package: {} at {}", name, package->path());
        pkg::setupPackageBuildLayout(*workspace, *package);
    }

    auto closure = pkg::dependencyClosure(*workspace, "image");
    for (const auto& package : closure) {
        LOG_INFO(gLogger, " - {}", package->name());
    }

    auto state = pkg::IWorkspaceState::ofSqlite(configPath.parent_path() / "build/workspace.db");

    auto downloadClient = pkg::IDownloadClient::create(configPath.parent_path() / "build/packagecache");

    auto pkgtool = pkg::IPkgTool::create(workspace, state, downloadClient);
    pkgtool->createPackageEnvironment("image");

    return 0;
} catch (const std::exception& ex) {
    LOG_CRITICAL(gLogger, "Fatal error: {}", ex.what());
    return 1;
}

}

int main(int argc, const char **argv) {
    setupLogger();
    defer {
        gLogger->flush_log();
        quill::Backend::stop();
    };

    LIBXML_TEST_VERSION;
    defer { xmlCleanupParser(); };

    OPENSSL_init_crypto(OPENSSL_INIT_ADD_ALL_DIGESTS, nullptr);
    defer { OPENSSL_cleanup(); };

    return run(argc, argv);
}
