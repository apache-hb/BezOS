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
#include <quill/std/Vector.h>

namespace fs = std::filesystem;

namespace {
quill::Logger* gLogger;

class ArgOptions {
    static constexpr char kConfigKey[] = "--config";
    static constexpr char kProfileKey[] = "--profile";
    static constexpr char kWorkspaceKey[] = "--vsc-workspace";

    static constexpr char kFetchKey[] = "--fetch";
    static constexpr char kCloneKey[] = "--clone";
    static constexpr char kConfigureKey[] = "--configure";
    static constexpr char kBuildKey[] = "--build";
    static constexpr char kInstallKey[] = "--install";
    static constexpr char kRecursiveKey[] = "--recursive";

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

        parser.add_argument(kCloneKey)
            .help("List of packages to clone from git")
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

        parser.add_argument(kRecursiveKey)
            .help("Recursively process dependencies")
            .default_value(false)
            .implicit_value(true);

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

    std::vector<std::string> clonePackages() const {
        return parser.get<std::vector<std::string>>(kCloneKey);
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

    bool recursive() const {
        return parser.get<bool>(kRecursiveKey);
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
    quill::PatternFormatterOptions pattern{"%(time) [%(thread_id)] %(log_level:<6) %(short_source_location:<12) %(message)", "%Y-%m-%dT%H:%M:%S.%QmsZ", quill::Timezone::GmtTime};
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

    pkg::setupWorkspace(*workspace);

    auto state = pkg::IWorkspaceState::ofSqlite(configPath.parent_path() / "build/workspace.db");

    auto downloadClient = pkg::IDownloadClient::create(configPath.parent_path() / "build/packagecache");

    auto pkgtool = pkg::IPkgTool::create(workspace, state, downloadClient);

    auto cloneList = options.clonePackages();
    auto fetchList = options.fetchPackages();
    auto configureList = options.configurePackages();
    auto buildList = options.buildPackages();
    auto installList = options.installPackages();

    fetchList.insert(fetchList.end(), cloneList.begin(), cloneList.end());
    std::sort(fetchList.begin(), fetchList.end());
    fetchList.erase(std::unique(fetchList.begin(), fetchList.end()), fetchList.end());

    LOG_INFO(gLogger, "Clone list: {}", cloneList);
    LOG_INFO(gLogger, "Fetching {}", fetchList);
    LOG_INFO(gLogger, "Configuring {}", configureList);
    LOG_INFO(gLogger, "Building {}", buildList);
    LOG_INFO(gLogger, "Installing {}", installList);

    auto shouldClone = [&](const std::string& name) {
        return std::find(cloneList.begin(), cloneList.end(), name) != cloneList.end();
    };

    for (const auto& name : fetchList) {
        pkgtool->lowerPackageState(name, pkg::PackageState::eUnknown);

        for (const auto& depName : state->getReverseDependencies(name, pkg::DependencyScope::ePrivateDependency | pkg::DependencyScope::ePublicDependency)) {
            pkgtool->lowerPackageState(depName, pkg::PackageState::eUnknown);
        }
    }

    for (const auto& name : configureList) {
        pkgtool->lowerPackageState(name, pkg::PackageState::eFetched);

        for (const auto& depName : state->getReverseDependencies(name, pkg::DependencyScope::ePrivateDependency | pkg::DependencyScope::ePublicDependency)) {
            pkgtool->lowerPackageState(depName, pkg::PackageState::eFetched);
        }
    }

    for (const auto& name : buildList) {
        pkgtool->lowerPackageState(name, pkg::PackageState::eConfigured);

        for (const auto& depName : state->getReverseDependencies(name, pkg::DependencyScope::ePrivateDependency | pkg::DependencyScope::ePublicDependency)) {
            pkgtool->lowerPackageState(depName, pkg::PackageState::eConfigured);
        }
    }

    for (const auto& name : installList) {
        pkgtool->lowerPackageState(name, pkg::PackageState::eBuilt);

        for (const auto& depName : state->getReverseDependencies(name, pkg::DependencyScope::ePrivateDependency | pkg::DependencyScope::ePublicDependency)) {
            pkgtool->lowerPackageState(depName, pkg::PackageState::eBuilt);
        }
    }

    for (const auto& fetchName : fetchList) {
        for (const auto& depName : pkg::totalDependencyClosure(*workspace, fetchName)) {
            LOG_INFO(gLogger, "Fetching package '{}'", depName->name());
            pkgtool->fetchPackageIfNeeded(depName->name());
        }

        LOG_INFO(gLogger, "Fetching package '{}'", fetchName);
        pkgtool->fetchPackageIfNeeded(fetchName, shouldClone(fetchName));
        LOG_INFO(gLogger, "Fetched package '{}'", fetchName);
    }

    for (const auto& configureName : configureList) {
        for (const auto& depName : pkg::totalDependencyClosure(*workspace, configureName)) {
            LOG_INFO(gLogger, "Configuring package '{}'", depName->name());
            pkgtool->configurePackageIfNeeded(depName->name());
        }

        LOG_INFO(gLogger, "Configuring package '{}'", configureName);
        pkgtool->configurePackageIfNeeded(configureName);
    }

    for (const auto& buildName : buildList) {
        for (const auto& depName : pkg::totalDependencyClosure(*workspace, buildName)) {
            LOG_INFO(gLogger, "Building package '{}'", depName->name());
            pkgtool->buildPackageIfNeeded(depName->name());
        }

        LOG_INFO(gLogger, "Building package '{}'", buildName);
        pkgtool->buildPackageIfNeeded(buildName);
    }

    for (const auto& installName : installList) {
        for (const auto& depName : pkg::totalDependencyClosure(*workspace, installName)) {
            LOG_INFO(gLogger, "Installing package '{}'", depName->name());
            pkgtool->installPackageIfNeeded(depName->name());
        }

        LOG_INFO(gLogger, "Installing package '{}'", installName);
        pkgtool->installPackageIfNeeded(installName);
    }

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
