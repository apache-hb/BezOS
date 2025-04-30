#include <argparse/argparse.hpp>

#include <subprocess.hpp>

#include <curl/curl.h>
#include <archive.h>
#include <archive_entry.h>

#include <argo.hpp>

#include <indicators/indeterminate_progress_bar.hpp>
#include <indicators/progress_bar.hpp>

#include <SQLiteCpp/SQLiteCpp.h>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xinclude.h>

#include <openssl/sha.h>
#include <openssl/crypto.h>

#include <glob/glob.h>

#include "defer.hpp"

#include <sys/mman.h>
#include <sys/mount.h>
#include <utime.h>
#include <sys/stat.h>

#include <filesystem>
#include <ranges>
#include <generator>
#include <iostream>
#include <fstream>

using namespace std::literals;

namespace fs = std::filesystem;
namespace sp = subprocess;
namespace stdr = std::ranges;
namespace stdv = std::views;
namespace sql = SQLite;

static fs::path gRepoRoot;
static fs::path gBuildRoot;
static fs::path gSourceRoot;
static fs::path gInstallPrefix;

static fs::path PackageCacheRoot() {
    return fs::absolute(gBuildRoot / "cache");
}

static fs::path PackageBuildRoot() {
    return fs::absolute(gBuildRoot / "packages");
}

static fs::path PackageImportRoot() {
    return fs::absolute(gBuildRoot / "sources");
}

static fs::path PackageLogRoot() {
    return fs::absolute(gBuildRoot / "logs");
}

static fs::path PackageCachePath(const std::string &name) {
    return PackageCacheRoot() / name;
}

static fs::path PackageBuildPath(const std::string &name) {
    return PackageBuildRoot() / name;
}

static fs::path PackageInstallPath(const std::string &name) {
    return fs::absolute(gInstallPrefix / name);
}

static fs::path PackageLogPath(const std::string &name) {
    return PackageLogRoot() / name;
}

static fs::path PackageImportPath(const std::string &name) {
    return PackageImportRoot() / name;
}

static void MakeFolder(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        if (!std::filesystem::create_directories(path)) {
            throw std::runtime_error("Failed to create directory " + path.string());
        }
    }
}

class Logger {
    std::ostream *mOutStream = &std::cout;
    std::ostream *mErrorStream = &std::cerr;

public:
    bool mVerbose = false;

    void OpenAsDaemon() {
        std::ofstream *out = new std::ofstream("/etc/pkgtool.log", std::ios::app);
        std::ofstream *err = new std::ofstream("/etc/pkgtool.err", std::ios::app);
        if (!out->is_open() || !err->is_open()) {
            throw std::runtime_error("Failed to open log files");
        }

        mOutStream = out;
        mErrorStream = err;
    }

    void logv(const std::string& message) {
        if (mVerbose) {
            *mOutStream << message << std::endl;
        }
    }

    void log(const std::string& message) {
        *mOutStream << message << std::endl;
    }

    void err(const std::string& message) {
        *mErrorStream << message << std::endl;
    }

    template<typename... A>
    void logf(const std::format_string<A...>& fmt, A&&... args) {
        log(std::vformat(fmt.get(), std::make_format_args(args...)));
    }

    template<typename... A>
    void errf(const std::format_string<A...>& fmt, A&&... args) {
        err(std::vformat(fmt.get(), std::make_format_args(args...)));
    }

    template<typename... A>
    void verbosef(const std::format_string<A...>& fmt, A&&... args) {
        if (mVerbose) {
            logv(std::vformat(fmt.get(), std::make_format_args(args...)));
        }
    }
};

static Logger logger{};

struct RequirePackage {
    std::string name;
    fs::path symlink;
};

struct Overlay {
    std::string folder;
};

struct GitRepo {
    std::string url;
    std::string branch;
    std::string commit;
};

struct Download {
    std::string name;
    std::string url;
    std::string file;
    std::string archive;
    std::optional<std::string> sha256;
    bool trimRootFolder;
    bool install;

    GitRepo git;
};

enum ConfigureProgram {
    eConfigureNone,

    eMeson,
    eCMake,
    eShell, // run a shell script
    eScript,
    eAutoconf,
};

struct ScriptExec {
    fs::path script;
    std::vector<std::string> args;
    std::map<std::string, std::string> env;
};

struct ConfigureStep {
    // type of configure step
    ConfigureProgram configure{eConfigureNone};

    // path to shell script to use if shell configure step
    fs::path shellScript;

    // args to pass to shell script
    std::vector<std::string> args;

    // environment variables to set
    std::map<std::string, std::string> env;

    // options to pass to configure tool
    std::map<std::string, std::string> options;

    // path to the configure tool if not default
    std::string configureToolPath;

    // path to the source folder to configure from if not toplevel
    std::string configureSourcePath;

    // path to meson/cmake cross file
    std::string crossFile;

    // path to meson/cmake native file
    std::string nativeFile;

    std::string scriptBody;
};

struct BuildStep {
    ConfigureProgram program{eConfigureNone};

    std::string targets;

    std::map<std::string, std::string> env;

    std::map<std::string, std::string> options;

    std::vector<std::string> args;
};

struct InstallStep {
    ConfigureProgram program{eConfigureNone};

    std::string targets;

    std::map<std::string, std::string> env;

    std::map<std::string, std::string> options;

    std::vector<std::string> args;
};

struct PackageInfo {
    std::string name;
    fs::path source;
    fs::path imported;
    fs::path cache;

    // package version
    std::string version;

    // files to download
    std::vector<Download> downloads;

    // folders to overlay after downloads are extracted
    std::vector<Overlay> overlays;

    // git diffs to apply after applying overlays
    std::vector<fs::path> patches;

    // packages to build before configuring
    std::vector<RequirePackage> dependencies;

    std::vector<ConfigureStep> configureSteps;

    std::vector<ScriptExec> scripts;

    std::vector<std::string> artifacts;

    std::string installTargets;

    std::string installName;

    std::string fromSource;

    std::string GetConfigureSourcePath(const ConfigureStep& step) const {
        return step.configureSourcePath.empty() ? GetSourceFolder().string() : step.configureSourcePath;
    }

    fs::path GetSourceFolder() const {
        if (source.empty() && imported.empty()) {
            throw std::runtime_error(std::format("Package {} has no source or imported folder", name));
        }

        return fs::absolute(source.empty() ? imported : source);
    }

    fs::path GetBuildFolder() const {
        return PackageBuildPath(name);
    }

    fs::path GetInstallPath() const {
        if (installName.empty()) {
            return PackageInstallPath(name);
        } else {
            return PackageInstallPath(installName);
        }
    }

    fs::path GetCacheFolder() const {
        return PackageCachePath(name);
    }

    ConfigureProgram guessBuildProgram() const {
        if (configureSteps.empty()) {
            return eConfigureNone;
        }

        return configureSteps.back().configure;
    }

    bool HasCompileCommands() const {
        return fs::exists(GetBuildFolder() / "compile_commands.json");
    }

    std::tuple<fs::path, fs::path> GetLogFiles(const std::string& stage) const {
        MakeFolder(PackageLogPath(name));
        auto out = PackageLogPath(name) / (stage + ".out");
        auto err = PackageLogPath(name) / (stage + ".err");
        fs::remove(out);
        fs::remove(err);

        return {out, err};
    }
};

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };

template<typename... Args>
static int execute(const std::vector<std::string>& cmd, Args&&... args) {
    if (logger.mVerbose) {
        logger.logf("{}", (cmd | stdv::join_with(' ') | stdr::to<std::string>()));

        if constexpr (sizeof...(args) > 0) {
            auto visitor = overloaded {
                [&](const sp::cwd& arg) {
                    logger.logf("cwd: {}", arg.arg_value);
                },
                [&](const sp::environment& arg) {
                    for (const auto& [key, value] : arg.env_) {
                        logger.logf("env: {}={}", key, value);
                    }
                },
                [&](auto&& arg) { }
            };

            (visitor(std::forward<Args>(args)), ...);
        }
    }
    return sp::call(cmd, std::forward<Args>(args)...);
}

constexpr static void ReplaceAll(std::string& str, std::string_view from, std::string_view to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

constexpr static std::string ReplaceText(std::string str, std::string_view from, std::string_view to) {
    ReplaceAll(str, from, to);
    return str;
}

static_assert(ReplaceText("@REPO@/data/image.sh", "@REPO@", "/repo") == "/repo/data/image.sh");

static void ReplacePathPlaceholders(std::string& str) {
    ReplaceAll(str, "@PREFIX@", fs::absolute(gInstallPrefix).string());
    ReplaceAll(str, "@BUILD@", fs::absolute(gBuildRoot).string());
    ReplaceAll(str, "@REPO@", fs::absolute(gRepoRoot).string());
    ReplaceAll(str, "@SOURCE@", fs::absolute(gSourceRoot).string());
}

static void ReplacePackagePlaceholders(std::string& str, const PackageInfo& package) {
    ReplacePathPlaceholders(str);
    ReplaceAll(str, "@PACKAGE@", package.name);
    ReplaceAll(str, "@SOURCE_ROOT@", package.GetSourceFolder().string());
    ReplaceAll(str, "@BUILD_ROOT@", package.GetBuildFolder().string());
    ReplaceAll(str, "@INSTALL_ROOT@", PackageInstallPath(package.name).string());
}

enum PackageStatus {
    eUnknown,
    eDownloaded,
    eConfigured,
    eBuilt,
    eInstalled,
};

static const char *GetPackageStatusString(PackageStatus status) {
    switch (status) {
    case eUnknown:
        return "unknown";
    case eDownloaded:
        return "downloaded";
    case eConfigured:
        return "configured";
    case eBuilt:
        return "built";
    case eInstalled:
        return "installed";
    default:
        return "unknown";
    }
}

static PackageStatus GetPackageStatusFromString(std::string_view status) {
    if (status == "unknown") {
        return eUnknown;
    } else if (status == "downloaded") {
        return eDownloaded;
    } else if (status == "configured") {
        return eConfigured;
    } else if (status == "built") {
        return eBuilt;
    } else if (status == "installed") {
        return eInstalled;
    } else {
        return eUnknown;
    }
}

struct XmlNode {
    xmlNodePtr node;

    XmlNode(xmlNodePtr node) : node(node) {}

    std::generator<XmlNode> children() const {
        for (xmlNodePtr child = node->children; child != nullptr; child = child->next) {
            co_yield child;
        }
    }

    std::map<std::string, std::string> properties() const {
        std::map<std::string, std::string> result;

        for (xmlAttrPtr attr = node->properties; attr != nullptr; attr = attr->next) {
            const xmlChar *name = attr->name;
            xmlChar *value = xmlGetProp(node, name);

            result[reinterpret_cast<const char *>(name)] = reinterpret_cast<const char *>(value);

            xmlFree(value);
        }

        return result;
    }

    std::optional<std::string> property(std::string name) const {
        xmlChar *value = xmlGetProp(node, reinterpret_cast<const xmlChar *>(name.c_str()));
        if (value == nullptr) {
            return std::nullopt;
        }

        defer { xmlFree(value); };

        return reinterpret_cast<const char *>(value);
    }

    std::string content() const {
        auto data = xmlNodeGetContent(node);

        if (data == nullptr) {
            throw std::runtime_error(std::format("Node {} has no content", name()));
        }

        return reinterpret_cast<const char *>(data);
    }

    std::string_view name() const {
        return reinterpret_cast<const char *>(node->name);
    }

    operator xmlNodePtr() const { return node; }
};

static std::string_view NodeName(xmlNodePtr node) {
    return reinterpret_cast<const char *>(node->name);
}

static auto IsXmlNodeOf(xmlElementType type) {
    return [type](xmlNodePtr node) { return node->type == type; };
}

struct Scope {
    void SubstituteVariables(std::string& str) {
        for (const auto& [name, value] : variables) {
            ReplaceAll(str, "${" + name + "}", value);
        }

        if (parent) {
            parent->SubstituteVariables(str);
        }
    }

    Scope *parent = nullptr;
    std::map<std::string, std::string> variables;

    Scope() = default;
    Scope(Scope *parent) : parent(parent) {}
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
    Scope(Scope&&) = default;
    Scope& operator=(Scope&&) = default;

    bool TryGetVariable(std::string name, std::string& result) {
        if (variables.contains(name)) {
            result = variables[name];
            return true;
        }

        if (parent) {
            return parent->TryGetVariable(name, result);
        }

        return false;
    }

    std::string GetVariable(std::string name) {
        std::string result;
        if (!TryGetVariable(name, result)) {
            throw std::runtime_error(std::format("Variable {} not found in scope", name));
        }

        return result;
    }

    void AddVariable(std::string name, std::string value) {
        std::string result;
        if (TryGetVariable(name, result)) {
            throw std::runtime_error(std::format("Variable {} already exists in scope", name));
        }

        if (name.empty()) {
            throw std::runtime_error("Variable name cannot be empty");
        }

        SubstituteVariables(value);

        variables[name] = value;
    }

    std::optional<std::string> TryGetProperty(XmlNode node, std::string_view key) {
        auto prop = node.property(std::string(key));
        if (prop) {
            auto text = *prop;
            SubstituteVariables(text);
            return text;
        }

        return std::nullopt;
    }

    std::string GetProperty(XmlNode node, std::string_view key) {
        auto prop = TryGetProperty(node, key);
        if (prop) {
            return *prop;
        }

        throw std::runtime_error(std::format("Property {} not found in node", key));
    }
};

class PackageDb {
    sql::Database mDatabase;

public:
    PackageDb(const fs::path& path) : mDatabase(path, sql::OPEN_READWRITE | sql::OPEN_CREATE) {
        mDatabase.exec(
            "CREATE TABLE IF NOT EXISTS targets (\n"
            "    name TEXT PRIMARY KEY,\n"
            "    status TEXT\n"
            ")\n"
        );

        mDatabase.exec("DELETE FROM targets WHERE name = ''");

        mDatabase.exec("DROP TABLE IF EXISTS dependencies");

        mDatabase.exec(
            "CREATE TABLE dependencies (\n"
            "    package TEXT,\n"
            "    dependency TEXT,\n"
            "    UNIQUE(package, dependency)\n"
            ")\n"
        );

        mDatabase.exec(
            "CREATE TABLE IF NOT EXISTS tools (\n"
            "    name TEXT PRIMARY KEY,\n"
            "    installed INTEGER NOT NULL\n"
            ")\n"
        );
    }

    bool HasTool(const std::string& name) {
        sql::Statement query(mDatabase, "SELECT 1 FROM tools WHERE name = ? AND installed = 1");
        query.bind(1, name);
        return query.executeStep() && query.getColumn(0).getInt() == 1;
    }

    void AddTool(const std::string& name) {
        sql::Statement query(mDatabase, "INSERT OR REPLACE INTO tools (name, installed) VALUES (?, 1)");
        query.bind(1, name);
        query.exec();
    }

    void AddDependency(std::string_view package, std::string_view dependency) {
        sql::Statement query(mDatabase, "INSERT OR REPLACE INTO dependencies (package, dependency) VALUES (?, ?)");
        query.bind(1, std::string(package));
        query.bind(2, std::string(dependency));
        query.exec();
    }

    /// @brief Walk up the dependency tree to find all dependant packages
    std::set<std::string> GetDependantPackages(std::string_view name) {
        std::set<std::string> result;

        sql::Statement query(mDatabase,
            "WITH RECURSIVE dependants AS (\n"
            "    SELECT package, dependency FROM dependencies WHERE dependency = ?\n"
            "    UNION\n"
            "    SELECT d.package, d.dependency FROM dependencies d\n"
            "    JOIN dependants ON d.dependency = dependants.package\n"
            ")\n"
            "SELECT package FROM dependants"
        );
        query.bind(1, std::string(name));

        while (query.executeStep()) {
            result.insert(query.getColumn(0).getString());
        }

        result.insert(std::string(name));

        return result;
    }

    std::set<std::string> GetPackageDependencies(std::string_view name) {
        std::set<std::string> result;

        sql::Statement query(mDatabase, "SELECT dependency FROM dependencies WHERE package = ?");
        query.bind(1, std::string(name));

        while (query.executeStep()) {
            result.insert(query.getColumn(0).getString());
        }

        return result;
    }

    /// @brief Order the packages in the correct build order
    std::vector<std::string> OrderPackages() {

        // gather top level packages
        std::set<std::string> root;

        // find all packages that have no dependant packages
        sql::Statement query(mDatabase,
            "SELECT name FROM targets\n"
            "WHERE NOT EXISTS (\n"
            "    SELECT 1 FROM dependencies WHERE dependency = targets.name\n"
            ")"
        );

        while (query.executeStep()) {
            root.insert(query.getColumn(0).getString());
        }

        std::vector<std::string> result;

        // workaround for clang bug, lambdas with deducing this cannot access
        // member variables.
        auto getPackageDependencies = [&](const auto& id) {
            return GetPackageDependencies(id);
        };

        auto visit = [&](this auto&& self, const std::string& name) {
            if (stdr::contains(result, name)) {
                return;
            }

            auto dependants = getPackageDependencies(name);
            for (const auto& dep : dependants) {
                self(dep);
            }

            result.push_back(name);
        };

        for (const auto& name : root) {
            visit(name);
        }

        return result;
    }

    /// @brief Get all packages that have no dependant packages
    std::set<std::string> GetToplevelPackages() {
        std::set<std::string> result;

        sql::Statement query(mDatabase,
            "SELECT name FROM targets\n"
            "WHERE NOT EXISTS (\n"
            "    SELECT 1 FROM dependencies WHERE dependency = targets.name\n"
            ")"
        );

        while (query.executeStep()) {
            result.insert(query.getColumn(0).getString());
        }

        return result;
    }

    std::set<std::string> GetPackageDependencies(const std::string& name) {
        std::set<std::string> result;

        sql::Statement query(mDatabase, "SELECT dependency FROM dependencies WHERE package = ?");
        query.bind(1, name);

        while (query.executeStep()) {
            result.insert(query.getColumn(0).getString());
        }

        return result;
    }

    PackageStatus GetPackageStatus(std::string_view name) {
        sql::Statement query(mDatabase, "SELECT status FROM targets WHERE name = ?");
        query.bind(1, std::string(name));

        if (query.executeStep()) {
            auto state = query.getColumn(0).getString();
            return GetPackageStatusFromString(state);
        }

        return eUnknown;
    }

    void RemovePackage(std::string_view name) {
        sql::Statement query(mDatabase, "DELETE FROM targets WHERE name = ?");
        query.bind(1, std::string(name));
        query.exec();
    }

    bool ShouldRunStep(std::string_view name, PackageStatus status) {
        auto current = GetPackageStatus(name);
        return current == eUnknown || current < status;
    }

    void SetPackageStatus(std::string_view name, PackageStatus status) {
        if (name.empty()) throw std::runtime_error("Empty package name");
        sql::Statement query(mDatabase, "INSERT OR REPLACE INTO targets (name, status) VALUES (?, ?)");
        query.bind(1, std::string(name));
        query.bind(2, GetPackageStatusString(status));
        query.exec();
    }

    void LowerPackageStatus(std::string_view name, PackageStatus status) {
        if (name.empty()) return;

        auto current = GetPackageStatus(name);
        if (current > status) {
            SetPackageStatus(name, status);
        }
    }

    void RaiseTargetStatus(std::string_view name, PackageStatus status) {
        if (name.empty()) return;

        auto current = GetPackageStatus(name);
        if (current == eUnknown || current < status) {
            SetPackageStatus(name, status);
        }
    }

    void DumpTargetStates() {
        sql::Statement query(mDatabase, "SELECT name, status FROM targets");
        while (query.executeStep()) {
            std::cout << query.getColumn(0).getString() << ": " << query.getColumn(1).getString() << std::endl;
        }
    }
};

static PackageDb *gPackageDb;

struct Workspace {
    std::map<std::string, PackageInfo> packages;
    bool removeOrphans{false};

    auto& getPackage(const std::string& name) {
        if (!packages.contains(name)) {
            throw std::runtime_error("Unknown package " + name);
        }

        return packages.at(name);
    }

    void RelinkPackage(const std::string& name) {
        PackageInfo info;
        if (!TryGetPackage(name, info)) {
            return;
        }

        for (auto& path : info.artifacts) {
            ReplacePackagePlaceholders(path, info);
        }

        for (auto path : glob::rglob(info.artifacts)) {
            fs::remove(path);
        }
    }

    bool TryGetPackage(const std::string& name, PackageInfo& info) {
        if (!packages.contains(name)) {
            if (removeOrphans) {
                gPackageDb->RemovePackage(name);
                logger.logf("Removed orphan package {}", name);
                return false;
            } else {
                throw std::runtime_error("Unknown package " + name + ". rerun with --remove-orphans to remove");
            }
        }

        info = packages.at(name);
        return true;
    }

    argo::json workspace{argo::json::object_e};

    void AddPackage(PackageInfo info) {
        if (packages.contains(info.name)) {
            throw std::runtime_error("Duplicate package " + info.name);
        }

        packages.emplace(info.name, info);

        argo::json folder{argo::json::object_e};
        folder["path"] = info.GetSourceFolder();
        folder["name"] = info.name;

        workspace["folders"].append(folder);
    }

    void AddFolder(const fs::path& path, const std::string& name) {
        argo::json folder{argo::json::object_e};
        folder["path"] = path.string();
        folder["name"] = name;

        workspace["folders"].append(folder);
    }

    fs::path GetPackagePath(const std::string& name) {
        if (!packages.contains(name)) {
            throw std::runtime_error("Unknown target " + name);
        }

        return getPackage(name).GetSourceFolder();
    }

    fs::path GetArtifactPath(const std::string& name);

    bool HasBuildTarget(const std::string& name) {
        return packages.contains(name);
    }

    /// @brief Order the packages in the correct build order
    std::vector<PackageInfo> OrderPackages() {
        std::vector<PackageInfo> result;

        std::set<std::string> visited;

        const auto& pkgs = this->packages;

        auto visit = [&](this auto&& self, const std::string& name) {
            if (visited.contains(name)) {
                return;
            }

            visited.insert(name);

            assert(pkgs.contains(name) && "Unknown package");
            auto& package = pkgs.at(name);
            for (const auto& dep : package.dependencies) {
                self(dep.name);
            }

            result.push_back(package);
        };

        for (const auto& [name, _] : packages) {
            visit(name);
        }

        return result;
    }
};

static Workspace gWorkspace;

template<typename T>
static T UnwrapOptional(std::optional<T> opt, std::string_view message) {
    if (!opt.has_value()) {
        throw std::runtime_error(std::string(message));
    }

    return opt.value();
}

template<typename T>
static T ExpectProperty(XmlNode node, const char *name) {
    return UnwrapOptional(node.property(name), "Node " + std::string(node.name()) + " is missing required property " + std::string(name));
}

using XmlDocument = std::unique_ptr<xmlDoc, decltype(&xmlFreeDoc)>;

static XmlDocument XmlDocumentOpen(const std::filesystem::path &path) {
    xmlDocPtr document = xmlReadFile(path.c_str(), nullptr, XML_PARSE_XINCLUDE);
    if (document == nullptr) {
        throw std::runtime_error("Failed to parse " + path.string());
    }

    XmlDocument doc = {document, xmlFreeDoc};

    if (xmlXIncludeProcess(doc.get()) == -1) {
        throw std::runtime_error("Failed to process xinclude in " + path.string());
    }

    return doc;
}

static std::set<std::string> gCloneRepos;

static void GitClone(const std::string& url, const std::string& commit, const std::string& branch, const fs::path& dst) {
    std::vector<std::string> args = { "git", "clone", url, fs::absolute(dst).string() };
    if (!branch.empty()) {
        args.push_back("--branch");
        args.push_back(branch);
    }

    auto result = execute(args);
    if (result != 0) {
        throw std::runtime_error("Failed to clone " + url);
    }

    if (!commit.empty()) {
        auto cwd = fs::current_path();
        fs::current_path(dst);

        auto result = execute({ "git", "checkout", commit });
        if (result != 0) {
            throw std::runtime_error("Failed to checkout commit " + commit);
        }

        fs::current_path(cwd);
    }
}

#define ARCHIVE_ASSERT(expr, archive) \
    if ((expr) != ARCHIVE_OK) { \
        std::string message = "Archive error: " #expr " "; \
        throw std::runtime_error(message + archive_error_string(archive)); \
    }

static void CopyData(struct archive *a, std::ostream& os) {
    const void *buff;
    size_t size;
    int64_t offset;

    while (archive_read_data_block(a, &buff, &size, &offset) == ARCHIVE_OK) {
        os.write(reinterpret_cast<const char *>(buff), size);
    }
}

static void ExtractArchive(std::string_view name, const fs::path& archive, const fs::path& dst, bool trimRootFolder) {
    fs::remove_all(dst);
    fs::create_directories(dst);

    struct archive *a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);

    if (archive_read_open_filename(a, archive.c_str(), 10240) != ARCHIVE_OK) {
        throw std::runtime_error("Failed to open archive " + archive.string() + " " + (archive_error_string(a) ?: "unknown error"));
    }

    defer {
        archive_read_close(a);
        archive_read_free(a);
    };

    auto fname = archive.filename().string();

    indicators::IndeterminateProgressBar bar{
        indicators::option::BarWidth{40},
        indicators::option::Start{"["},
        indicators::option::Lead{"*"},
        indicators::option::End{"]"},
        indicators::option::PrefixText{std::format("Extracting {} ", fname)},
        indicators::option::ForegroundColor{indicators::Color::white},
        indicators::option::FontStyles{std::vector<indicators::FontStyle>{indicators::FontStyle::bold}}
    };

    struct archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char *it = archive_entry_pathname(entry);
        if (it == nullptr) {
            throw std::runtime_error("Failed to get entry path");
        }
        std::string entryPath = it;

        if (trimRootFolder) {
            entryPath = entryPath.substr(entryPath.find('/') + 1);
        }

        bar.tick();

        if (entryPath.empty()) {
            continue;
        }

        if (entryPath.ends_with('/')) {
            fs::create_directories(dst / entryPath);
            continue;
        }

        fs::path path = entryPath;
        fs::path file = dst / path;

        {
            std::ofstream os(file, std::ios::binary);
            if (!os.is_open()) {
                throw std::runtime_error("Failed to open file "s + path.string());
            }

            if (archive_entry_size(entry) > 0) {
                CopyData(a, os);
            }
        }

        // preserve mtime
        auto mtime = archive_entry_mtime(entry);
        if (mtime > 0) {
            struct stat times;
            stat(file.c_str(), &times);

            struct utimbuf utimes;
            utimes.actime = times.st_atime;
            utimes.modtime = mtime / 1000000;
            utime(file.c_str(), &utimes);
        }

        // preserve permissions
        auto mode = archive_entry_mode(entry);
        if (mode > 0) {
            chmod(file.c_str(), mode);
        }

        int count = archive_file_count(a);
        bar.set_option(indicators::option::PostfixText{std::format("{}", count)});
    }

    bar.mark_as_completed();
}

static void VerifySha256(const fs::path& path, const std::string& expected) {
    FILE *file = fopen(path.c_str(), "rb");
    if (file == nullptr) {
        throw std::runtime_error("Failed to open file " + path.string());
    }
    defer { fclose(file); };

    size_t size = fs::file_size(path);

    void *ptr = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fileno(file), 0);
    if (ptr == MAP_FAILED) {
        throw std::runtime_error("Failed to map file " + path.string());
    }

    unsigned char buffer[SHA256_DIGEST_LENGTH];
    unsigned char *sha = SHA256((const unsigned char *)ptr, size, buffer);
    if (sha == nullptr) {
        throw std::runtime_error("Failed to calculate sha256 for " + path.string());
    }

    std::string hash;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        hash += std::format("{:02x}", buffer[i]);
    }

    if (hash != expected) {
        throw std::runtime_error("Hash mismatch for " + path.string() + ": expected " + expected + " got " + hash);
    }
}

static void DownloadFile(const std::string& url, const fs::path& dst) {
    CURL *curl = curl_easy_init();
    if (curl == nullptr) {
        throw std::runtime_error("Failed to initialize curl");
    }

    defer { curl_easy_cleanup(curl); };

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    FILE *file = fopen(dst.c_str(), "wb");
    if (file == nullptr) {
        throw std::runtime_error("Failed to open file " + dst.string());
    }

    defer { fclose(file); };

    indicators::IndeterminateProgressBar bar{
        indicators::option::BarWidth{40},
        indicators::option::Start{"["},
        indicators::option::Lead{"*"},
        indicators::option::End{"]"},
        indicators::option::PrefixText{std::format("Downloading {} ", url)},
        indicators::option::ForegroundColor{indicators::Color::white},
        indicators::option::FontStyles{std::vector<indicators::FontStyle>{indicators::FontStyle::bold}}
    };

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &bar);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, +[](void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
        if (dlnow == 0 || dltotal == 0) {
            return 0;
        }

        auto bar = static_cast<indicators::IndeterminateProgressBar *>(clientp);
        bar->set_option(indicators::option::PostfixText{std::format("{} / {}", dlnow, dltotal)});
        return 0;
    });

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to download " + url + ": " + (curl_easy_strerror(res) ?: "unknown error"));
    }
}

static bool ReadRequireTag(XmlNode node, Scope& scope, const std::string& name, std::vector<RequirePackage>& deps) {
    auto package = scope.GetProperty(node, "package");
    auto symlink = scope.TryGetProperty(node, "symlink");

    if (package == name) {
        throw std::runtime_error("Package " + name + " cannot require itself");
    }

    if (!gWorkspace.HasBuildTarget(package)) {
        throw std::runtime_error("Package " + name + " requires unknown package " + package);
    }

    gPackageDb->AddDependency(name, package);

    if (!symlink.has_value()) {
        return false;
    }

    deps.push_back(RequirePackage{package, symlink.value()});
    return true;
}

template<typename F>
static void EvalSwitchTag(XmlNode node, Scope& scope, F&& func) {
    auto on = scope.GetProperty(node, "on");
    for (XmlNode child : node.children() | stdv::filter(IsXmlNodeOf(XML_ELEMENT_NODE))) {
        if (child.name() == "case"sv) {
            auto value = scope.GetProperty(child, "it");
            if (value == on) {
                func(child);
                break;
            }
        }
    }
}

static void ReadConfigureBody(XmlNode action, ConfigureStep& step, Scope& scope) {
    for (XmlNode child : action.children() | stdv::filter(IsXmlNodeOf(XML_ELEMENT_NODE))) {
        if (child.name() == "options"sv) {
            for (const auto& [key, value] : child.properties()) {
                std::string option = value;
                scope.SubstituteVariables(option);
                if (step.options.contains(key)) {
                    throw std::runtime_error("Duplicate option " + key);
                }
                step.options[key] = option;
            }
        } else if (child.name() == "source"sv) {
            step.configureSourcePath = scope.GetProperty(child, "path");
        } else if (child.name() == "env"sv) {
            for (const auto& [key, value] : child.properties()) {
                std::string option = value;
                scope.SubstituteVariables(option);
                if (step.env.contains(key)) {
                    throw std::runtime_error("Duplicate env variable " + key);
                }
                step.env[key] = option;
            }
        } else if (child.name() == "arg"sv) {
            step.args.push_back(child.content());
        } else if (child.name() == "switch"sv) {
            EvalSwitchTag(child, scope, [&](XmlNode node) {
                ReadConfigureBody(node, step, scope);
            });
        }
    }
}

static ConfigureStep ReadConfigureStep(XmlNode action, const PackageInfo& packageInfo, Scope& scope) {
    ConfigureStep step{};

    auto with = scope.GetProperty(action, "with");
    if (with == "meson") {
        step.configure = eMeson;
        for (XmlNode child : action.children() | stdv::filter(IsXmlNodeOf(XML_ELEMENT_NODE))) {
            if (child.name() == "cross-file"sv) {
                step.crossFile = scope.GetProperty(child, "path");
            } else if (child.name() == "native-file"sv) {
                step.nativeFile = scope.GetProperty(child, "path");
            }
        }
    } else if (with == "cmake") {
        step.configure = eCMake;
    } else if (with == "shell") {
        step.configure = eShell;
        step.configureToolPath = scope.GetProperty(action, "path");
    } else if (with == "autoconf") {
        step.configure = eAutoconf;
    } else if (with == "script") {
        step.configure = eScript;
        step.scriptBody = action.content();
    } else {
        throw std::runtime_error("Unknown configure program " + with);
    }

    auto tool = scope.TryGetProperty(action, "using");
    if (tool.has_value()) {
        step.configureToolPath = tool.value();
        scope.SubstituteVariables(step.configureToolPath);
    }

    ReadConfigureBody(action, step, scope);

    return step;
}

static void AddWorkspacePackage(PackageInfo packageInfo, std::string name) {
    if (packageInfo.source.empty()) {
        auto source = PackageImportPath(name);
        MakeFolder(source);
        packageInfo.imported = source;
    }

    gWorkspace.AddPackage(packageInfo);
}

static Download ReadDownloadTags(XmlNode node, Scope& scope) {
    auto name = scope.TryGetProperty(node, "name").value_or("");
    auto url = scope.GetProperty(node, "url");
    auto file = scope.GetProperty(node, "file");
    auto archive = scope.TryGetProperty(node, "archive");
    auto trimRootFolder = scope.TryGetProperty(node, "trim-root-folder").value_or("false") == "true";
    auto install = scope.TryGetProperty(node, "install").value_or("false") == "true";
    auto sha256 = scope.TryGetProperty(node, "sha256");

    GitRepo repo;
    if (auto git = scope.TryGetProperty(node, "git")) {
        repo.url = git.value();
        repo.branch = scope.TryGetProperty(node, "branch").value_or("");
        repo.commit = scope.TryGetProperty(node, "commit").value_or("");
    }

    return Download{name, url, file, archive.value_or(""), sha256, trimRootFolder, install, repo};
}

static void ReadDownloadConfig(XmlNode root, Scope& scope) {
    auto name = scope.GetProperty(root, "name");
    gPackageDb->RaiseTargetStatus(name, eUnknown);

    auto download = ReadDownloadTags(root, scope);
    PackageInfo packageInfo {
        .name = name,
    };

    if (auto version = scope.TryGetProperty(root, "version")) {
        packageInfo.version = version.value();
    }

    packageInfo.downloads.push_back(download);

    for (XmlNode action : root.children() | stdv::filter(IsXmlNodeOf(XML_ELEMENT_NODE))) {
        auto step = NodeName(action);
        if (step == "patch"sv) {
            auto file = scope.GetProperty(action, "file");
            packageInfo.patches.push_back(file);
        } else if (step == "require"sv) {
            ReadRequireTag(action, scope, name, packageInfo.dependencies);
        } else if (step == "overlay"sv) {
            auto folder = scope.GetProperty(action, "path");
            packageInfo.overlays.push_back(Overlay{folder});
        }
    }

    AddWorkspacePackage(packageInfo, name);
}

static void ReadVarElement(XmlNode root, Scope& scope) {
    for (auto& [key, value] : root.properties()) {
        scope.AddVariable(key, value);
    }
}

static void ReadPackageBody(XmlNode root, PackageInfo& packageInfo, Scope& scope) {
    for (XmlNode action : root.children() | stdv::filter(IsXmlNodeOf(XML_ELEMENT_NODE))) {
        auto step = NodeName(action);
        if (step == "download"sv) {
            assert(packageInfo.fromSource.empty() && "Package cannot have both source and download steps");

            packageInfo.downloads.push_back(ReadDownloadTags(action, scope));
        } else if (step == "patch"sv) {
            assert(packageInfo.fromSource.empty() && "Package cannot have both source and patch steps");

            auto file = scope.GetProperty(action, "file");
            packageInfo.patches.push_back(file);
        } else if (step == "require"sv) {
            ReadRequireTag(action, scope, packageInfo.name, packageInfo.dependencies);
        } else if (step == "source"sv) {
            if (!packageInfo.source.empty()) {
                throw std::runtime_error("Package " + packageInfo.name + " already has source folder");
            }

            auto src = scope.GetProperty(action, "path");
            packageInfo.source = gSourceRoot / src;
        } else if (step == "overlay"sv) {
            assert(packageInfo.fromSource.empty() && "Package cannot have both source and overlay steps");

            auto folder = scope.GetProperty(action, "path");
            packageInfo.overlays.push_back(Overlay{folder});
        } else if (step == "configure"sv) {
            packageInfo.configureSteps.push_back(ReadConfigureStep(action, packageInfo, scope));
        } else if (step == "script"sv) {
            auto script = scope.GetProperty(action, "path");
            ScriptExec exec {
                .script = script
            };

            for (XmlNode child : action.children() | stdv::filter(IsXmlNodeOf(XML_ELEMENT_NODE))) {
                auto type = child.name();
                if (type == "arg"sv) {
                    exec.args.push_back(scope.GetProperty(child, "value"));
                } else if (type == "env"sv) {
                    auto key = scope.GetProperty(child, "key");
                    auto value = scope.GetProperty(child, "value");
                    exec.env[key] = value;
                }
            }

            packageInfo.scripts.push_back(exec);
        } else if (step == "install") {
            packageInfo.installTargets = scope.GetProperty(action, "targets");
        } else if (step == "artifacts") {
            auto files = scope.GetProperty(action, "files");
            for (auto file : files | stdv::split(' ')) {
                packageInfo.artifacts.push_back(std::string(file.begin(), file.end()));
            }
        } else if (step == "switch") {
            EvalSwitchTag(action, scope, [&](XmlNode child) {
                ReadPackageBody(child, packageInfo, scope);
            });
        }
    }
}

static void ReadPackageConfig(XmlNode root, Scope& outer) {
    auto name = outer.GetProperty(root, "name");
    gPackageDb->RaiseTargetStatus(name, eUnknown);

    Scope scope(&outer);

    PackageInfo packageInfo {
        .name = name,
    };

    if (auto installName = scope.TryGetProperty(root, "install-name")) {
        packageInfo.installName = installName.value();
    }

    if (auto from = scope.TryGetProperty(root, "from")) {
        packageInfo.fromSource = from.value();
        packageInfo.imported = gSourceRoot / from.value();
        gPackageDb->AddDependency(name, from.value());
    }

    if (auto version = scope.TryGetProperty(root, "version")) {
        packageInfo.version = version.value();
    }

    ReadPackageBody(root, packageInfo, scope);

    AddWorkspacePackage(packageInfo, name);
}

static void ReadArtifactConfig(XmlNode node, Scope& outer) {
    auto name = outer.GetProperty(node, "name");

    PackageInfo artifactInfo {
        .name = name
    };

    Scope scope(&outer);

    for (XmlNode action : node.children() | stdv::filter(IsXmlNodeOf(XML_ELEMENT_NODE))) {
        auto step = action.name();
        if (step == "require"sv) {
            ReadRequireTag(action, scope, name, artifactInfo.dependencies);
        } else if (step == "script"sv) {
            auto script = scope.GetProperty(action, "path");
            ScriptExec exec {
                .script = script
            };

            for (XmlNode child : action.children() | stdv::filter(IsXmlNodeOf(XML_ELEMENT_NODE))) {
                auto type = child.name();
                if (type == "arg"sv) {
                    exec.args.push_back(scope.GetProperty(child, "value"));
                } else if (type == "env"sv) {
                    auto key = scope.GetProperty(child, "key");
                    auto value = scope.GetProperty(child, "value");
                    exec.env[key] = value;
                }
            }

            artifactInfo.scripts.push_back(exec);
        }
    }

    if (artifactInfo.source.empty()) {
        auto source = PackageImportPath(name);
        MakeFolder(source);
        artifactInfo.imported = source;
    }

    gWorkspace.AddPackage(artifactInfo);
}

static void AcquirePackage(const PackageInfo& package) {
    assert(!package.name.empty() && "Package name cannot be empty");

    if (!gPackageDb->ShouldRunStep(package.name, eDownloaded)) {
        return;
    }

    logger.logf("{}: acquire", package.name);

    MakeFolder(package.GetCacheFolder());

    for (const auto& download : package.downloads) {
        if (gCloneRepos.contains(download.name)) {
            GitClone(download.git.url, download.git.commit, download.git.branch, package.GetSourceFolder());
        } else {
            auto path = package.GetCacheFolder() / download.file;

            logger.logf("{}: download {} -> {}", package.name, download.url, path.string());

            if (!fs::exists(path)) {
                DownloadFile(download.url, path);

                if (download.sha256.has_value()) {
                    logger.logf("{}: verify sha256 {}", package.name, download.sha256.value());
                    VerifySha256(path, download.sha256.value());
                }
            }

            if (!download.archive.empty()) {
                ExtractArchive(package.name, path, package.GetSourceFolder(), download.trimRootFolder);
            } else {
                auto dst = package.GetSourceFolder() / download.file;
                logger.logf("{}: copy {} -> {}", package.name, path.string(), dst.string());

                fs::remove_all(dst);
                fs::copy_file(path, dst, fs::copy_options::overwrite_existing);
            }
        }
    }

    for (const auto& overlay : package.overlays) {
        auto src = gSourceRoot / overlay.folder;
        fs::path dst = package.GetSourceFolder();

        logger.logf("{}: overlay {} -> {}", package.name, src.string(), dst.string());

        fs::copy(src, dst, fs::copy_options::recursive | fs::copy_options::update_existing);
    }

    for (const auto& patch : package.patches) {
        auto path = patch.string();
        ReplacePackagePlaceholders(path, package);

        logger.logf("{}: patch {}", package.name, path);

        std::vector<std::string> args = {
            "patch", "-p1", "-i", path
        };

        auto ws = package.GetSourceFolder().string();

        auto result = execute(args, sp::cwd{ws});
        if (result != 0) {
            throw std::runtime_error("Failed to apply patch " + patch.string());
        }
    }

    gPackageDb->RaiseTargetStatus(package.name, eDownloaded);
}

static void AddGitignoreSymlink(const fs::path& path) {
    std::fstream file(".gitignore", std::ios::in | std::ios::out);

    // read in the file
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    // if the file doesn't contain the symlink, add it
    std::string search = "\n/" + path.string() + "\n";
    if (content.find(search) == std::string::npos) {
        file << "/" + path.string() + "\n";
    }
}

static void ConnectDependencies(const PackageInfo& package) {
    for (const auto& dep : package.dependencies) {
        if (dep.symlink.empty()) continue;

        auto symlink = package.GetSourceFolder() / dep.symlink;
        auto path = gWorkspace.GetPackagePath(dep.name);

        auto cwd = fs::current_path();
        auto relative = symlink.lexically_relative(cwd);

        AddGitignoreSymlink(relative);

        if (fs::exists(symlink) && fs::equivalent(path, symlink)) {
            continue;
        }

        logger.logf("{}: symlink {} -> {}", package.name, symlink.string(), path.string());

        fs::create_directories(symlink.parent_path());
        fs::create_directory_symlink(path, symlink);
    }
}

static void ReplaceFilePlaceholders(const fs::path& path, const fs::path& dst, const PackageInfo& package) {
    std::ifstream file(path);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    ReplacePackagePlaceholders(content, package);

    std::ofstream out(dst);
    out << content;
}

static void RunConfigureStep(const PackageInfo& package, const ConfigureStep& step, int index) {
    if (step.configure == eConfigureNone) {
        return;
    }

    auto cwd = package.GetConfigureSourcePath(step);
    ReplacePackagePlaceholders(cwd, package);
    logger.logf("{}: configure {}", package.name, cwd);

    auto env = step.env;
    for (auto& [key, value] : env) {
        ReplacePackagePlaceholders(value, package);
    }
    env["PREFIX"] = fs::absolute(gInstallPrefix).string();
    env["BUILD"] = fs::absolute(gBuildRoot).string();
    env["REPO"] = fs::absolute(gRepoRoot).string();
    env["SOURCE"] = fs::absolute(gSourceRoot).string();

    auto [out, err] = package.GetLogFiles("configure");

    auto builddir = package.GetBuildFolder().string();
    auto cfgdir = fs::absolute(gBuildRoot / "configure" / package.name);
    fs::create_directories(cfgdir);

    if (step.configure == eMeson) {
        std::string configureTool = step.configureToolPath.empty() ? "meson" : step.configureToolPath;
        std::vector<std::string> args = {
            configureTool, "setup", builddir, "--wipe",
            "--prefix", package.GetInstallPath(),
        };

        if (!step.crossFile.empty()) {
            args.push_back("--cross-file");
            std::string crossFile = step.crossFile;
            ReplacePackagePlaceholders(crossFile, package);
            fs::path dst = cfgdir / "crossfile.txt";
            ReplaceFilePlaceholders(crossFile, dst, package);
            args.push_back(dst.string());
        }

        if (!step.nativeFile.empty()) {
            args.push_back("--native-file");
            std::string nativeFile = step.nativeFile;
            ReplacePackagePlaceholders(nativeFile, package);
            fs::path dst = cfgdir / "nativefile.txt";
            ReplaceFilePlaceholders(nativeFile, dst, package);
            args.push_back(dst.string());
        }

        for (auto& [key, value] : step.options) {
            std::string val = value;
            ReplacePackagePlaceholders(val, package);
            args.push_back("-D" + key + "=" + val);
        }

        auto result = execute(args, sp::cwd{cwd}, sp::environment{env});
        if (result != 0) {
            throw std::runtime_error("Failed to configure package " + package.name);
        }
    } else if (step.configure == eCMake) {
        std::string configureTool = step.configureToolPath.empty() ? "cmake" : step.configureToolPath;
        if (fs::exists(builddir)) {
            fs::remove_all(builddir);
        }

        std::vector<std::string> args = {
            configureTool, "-S", cwd, "-B", builddir,
            "-DCMAKE_INSTALL_PREFIX=" + package.GetInstallPath().string(),
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
            "-G", "Ninja",
            "-DCMAKE_BUILD_TYPE=MinSizeRel",
        };

        for (auto& [key, value] : step.options) {
            std::string val = value;
            ReplacePackagePlaceholders(val, package);
            args.push_back("-D" + key + "=" + val);
        }

        auto result = execute(args, sp::cwd{cwd}, sp::environment{env});
        if (result != 0) {
            throw std::runtime_error("Failed to configure package " + package.name);
        }
    } else if (step.configure == eAutoconf) {
        if (fs::exists(builddir)) {
            fs::remove_all(builddir);
        }

        fs::create_directories(builddir);

        std::vector<std::string> args = {
            "/bin/sh",
            (fs::current_path() / package.GetConfigureSourcePath(step) / "configure").string(),
            "--prefix=" + package.GetInstallPath().string()
        };

        for (auto& [key, value] : step.options) {
            std::string val = value;
            if (val.empty()) {
                args.push_back("--" + key);
            } else {
                ReplacePackagePlaceholders(val, package);
                args.push_back("--" + key + "=" + val);
            }
        }

        auto result = execute(args, sp::cwd{builddir}, sp::environment{env}, sp::output{out.c_str()}, sp::error{err.c_str()});
        if (result != 0) {
            throw std::runtime_error("Failed to configure package " + package.name);
        }
    } else if (step.configure == eShell) {
        auto path = step.configureToolPath;

        std::vector<std::string> args = { "/bin/sh", path };
        args.insert(args.end(), step.args.begin(), step.args.end());

        for (std::string& arg : args) {
            ReplacePackagePlaceholders(arg, package);
        }

        auto result = execute(args, sp::environment{env}, sp::cwd{builddir});
        if (result != 0) {
            throw std::runtime_error("Failed to run script " + args[1]);
        }
    } else if (step.configure == eScript) {
        std::string scriptBody = step.scriptBody;
        ReplacePackagePlaceholders(scriptBody, package);

        auto path = cfgdir / std::format("configure-{}.sh", index);
        {
            std::ofstream file(path);
            file << scriptBody;
        }

        std::vector<std::string> args = { "/bin/sh", path };
        auto result = execute(args, sp::environment{env}, sp::cwd{builddir});
        if (result != 0) {
            throw std::runtime_error("Failed to run script " + path.string());
        }
    } else {
        throw std::runtime_error("Unknown configure program for " + package.name);
    }
}

static void ConfigurePackage(const PackageInfo& package) {
    if (!gPackageDb->ShouldRunStep(package.name, eConfigured)) {
        return;
    }

    MakeFolder(package.GetBuildFolder());

    int index = 0;
    for (const auto& step : package.configureSteps) {
        RunConfigureStep(package, step, index++);
    }

    gPackageDb->RaiseTargetStatus(package.name, eConfigured);
}

static void BuildPackage(const PackageInfo& package) {
    if (!gPackageDb->ShouldRunStep(package.name, eBuilt)) {
        return;
    }

    logger.logf("{}: build", package.name);

    auto [out, err] = package.GetLogFiles("build");

    std::string builddir = package.GetBuildFolder().string();

    auto buildProgram = package.guessBuildProgram();

    if (buildProgram == eMeson) {
        logger.logf("{}: build program meson", package.name);
        auto result = execute({ "meson", "compile" }, sp::cwd{builddir});
        if (result != 0) {
            throw std::runtime_error("Failed to build package " + package.name);
        }
    } else if (buildProgram == eCMake) {
        logger.logf("{}: build program cmake", package.name);
        auto result = execute({ "cmake", "--build", builddir }, sp::cwd{builddir});
        if (result != 0) {
            throw std::runtime_error("Failed to build package " + package.name);
        }
    } else if (buildProgram == eAutoconf) {
        logger.logf("{}: build program autoconf", package.name);
        auto result = execute({ "make", "-j" + std::to_string(std::thread::hardware_concurrency()), "-Otarget" }, sp::cwd{builddir});
        if (result != 0) {
            throw std::runtime_error("Failed to build package " + package.name);
        }
    }

    gPackageDb->RaiseTargetStatus(package.name, eBuilt);
}

static void CopyInstallFiles(const PackageInfo& package) {
    for (const auto& download : package.downloads) {
        if (download.install) {
            logger.logf("{}: copy {}", package.name, download.file);

            auto path = package.GetSourceFolder() / download.file;
            auto dst = package.GetInstallPath() / download.file;

            fs::copy_file(path, dst, fs::copy_options::overwrite_existing);
        }
    }
}

static void InstallPackage(const PackageInfo& package) {
    auto buildProgram = package.guessBuildProgram();
    if (buildProgram == eConfigureNone) {
        CopyInstallFiles(package);
        return;
    }

    if (!gPackageDb->ShouldRunStep(package.name, eInstalled)) {
        return;
    }

    MakeFolder(package.GetInstallPath());

    logger.logf("{}: install", package.name);

    auto [out, err] = package.GetLogFiles("install");

    std::string builddir = package.GetBuildFolder().string();

    {
        if (buildProgram == eMeson) {
            logger.logf("{}: install with meson", package.name);
            std::vector<std::string> args = { "meson", "install", "--no-rebuild", "--skip-subprojects" };
            if (!package.installTargets.empty()) {
                args.push_back("--tags");
                args.push_back((package.installTargets | stdv::split(' ') | stdv::join_with(',')) | stdr::to<std::string>());
            }
            auto result = execute({ "meson", "install", "--no-rebuild", "--skip-subprojects" }, sp::cwd{builddir}, sp::output{out.c_str()}, sp::error{err.c_str()});
            if (result != 0) {
                throw std::runtime_error("Failed to install package " + package.name);
            }
        } else if (buildProgram == eCMake) {
            logger.logf("{}: install with cmake", package.name);
            auto result = execute({ "cmake", "--install", builddir }, sp::cwd{builddir}, sp::output{out.c_str()}, sp::error{err.c_str()});
            if (result != 0) {
                throw std::runtime_error("Failed to install package " + package.name);
            }
        } else if (buildProgram == eAutoconf) {
            std::vector<std::string> args = { "make" };
            if (package.installTargets.empty()) {
                args.push_back("install");
            } else {
                for (auto target : (package.installTargets | stdv::split(' '))) {
                    args.push_back(std::string(std::string_view(target)));
                }
            }
            auto result = execute(args, sp::cwd{builddir}, sp::output{out.c_str()}, sp::error{err.c_str()});
            if (result != 0) {
                throw std::runtime_error("Failed to install package " + package.name);
            }
        }
    }

    CopyInstallFiles(package);

    gPackageDb->RaiseTargetStatus(package.name, eInstalled);
}

static void GenerateArtifact(std::string_view name, const PackageInfo& artifact) {
    if (!gPackageDb->ShouldRunStep(name, eInstalled)) {
        return;
    }

    logger.logf("{}: generate", artifact.name);

    for (const auto& script : artifact.scripts) {
        auto path = script.script.string();

        std::vector<std::string> args = { "/bin/sh", path };
        args.insert(args.end(), script.args.begin(), script.args.end());

        for (std::string& arg : args) {
            ReplacePathPlaceholders(arg);
        }

        std::map env = script.env;
        env["PREFIX"] = gInstallPrefix.string();
        env["BUILD"] = gBuildRoot.string();
        env["REPO"] = gRepoRoot.string();
        env["SOURCE"] = gSourceRoot.string();

        logger.logf("{}: execute {}", name, args[1]);
        auto result = execute(args, sp::environment{env});
        if (result != 0) {
            throw std::runtime_error("Failed to run script " + args[1]);
        }
    }

    gPackageDb->RaiseTargetStatus(name, eInstalled);
}

static std::set<fs::path> gImportPaths;
static std::vector<fs::path> gIncludePaths;
static std::set<fs::path> gProcessedFiles;

static std::optional<fs::path> findInclude(const fs::path& path) {
    if (fs::exists(path)) {
        return path;
    }

    for (const auto& include : gIncludePaths) {
        fs::path inc = include / path;
        if (fs::exists(inc)) {
            return inc;
        }
    }

    return {};
}

static int incMatch(const char *uri) {
    return findInclude(uri).has_value();
}

static void *incOpen(const char *uri) {
    fs::path path = findInclude(uri).value();
    gProcessedFiles.emplace(path);
    gImportPaths.insert(path);
    return fopen(path.string().c_str(), "r");
}

static int incClose(void *context) {
    return fclose((FILE*)context);
}

static int incRead(void *context, char *buffer, int size) {
    return fread(buffer, 1, size, (FILE*)context);
}

static void GenerateClangDaemonConfig(const std::vector<std::string>& args) {
    auto shouldIncludeFragment = [&](const std::string& name) {
        if (args.empty()) return true;

        return stdr::find(args, name) != args.end();
    };

    std::string text = [&] {
        std::ifstream file(".clangd");

        // read in the file
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        auto addClangdSection = [&](const fs::path& builddir, const fs::path& workspace) {
            std::string fragment = std::format(
                "If:\n"
                "  PathMatch: [ {}/.* ]\n"
                "CompileFlags:\n"
                "  CompilationDatabase: {}\n",
                workspace.string(),
                builddir.string()
            );

            if (content.find(workspace.string()) == std::string::npos) {
                if (!content.empty()) {
                    content += "---\n";
                }

                content += fragment;
            }
        };

        for (const auto& [name, pkg] : gWorkspace.packages) {
            if (!shouldIncludeFragment(name)) {
                continue;
            }

            if (pkg.HasCompileCommands()) {
                auto workspace = pkg.GetSourceFolder().lexically_relative(fs::current_path());
                auto builddir = pkg.GetBuildFolder();

                addClangdSection(builddir, workspace);
            }
        }

        // TODO: this isnt great, but idk how to figure out the build folder for the build tool.
        addClangdSection(gBuildRoot / "tool", "tool");

        return content;
    }();

    // truncate the file and write the new content
    std::ofstream file(".clangd", std::ios::out | std::ios::trunc);
    file << text;
}

static void ReadRepoElement(XmlNode root, Scope& scope) {
    for (XmlNode child : root.children() | stdv::filter(IsXmlNodeOf(XML_ELEMENT_NODE))) {
        auto type = child.name();
        if (type == "package"sv) {
            ReadPackageConfig(child, scope);
        } else if (type == "download"sv) {
            ReadDownloadConfig(child, scope);
        } else if (type == "artifact"sv) {
            ReadArtifactConfig(child, scope);
        } else if (type == "collection"sv) {
            ReadRepoElement(child, scope);
        } else if (type == "var"sv) {
            ReadVarElement(child, scope);
        } else if (type == "switch"sv) {
            EvalSwitchTag(child, scope, [&](XmlNode node) {
                ReadRepoElement(node, scope);
            });
        } else {
            logger.errf("Unknown tag {}", type);
            assert(false && "Unknown tag");
        }
    }
}

static void ReadTargetElement(XmlNode root, Scope& scope) {
    for (XmlNode child : root.children() | stdv::filter(IsXmlNodeOf(XML_ELEMENT_NODE))) {
        auto type = child.name();
        if (type == "var"sv) {
            ReadVarElement(child, scope);
        } else {
            logger.errf("Unknown tag {}", type);
            assert(false && "Unknown tag");
        }
    }
}

static void VisitPackage(const PackageInfo& packageInfo) {
    assert(!packageInfo.name.empty() && "Package name cannot be empty");

    auto deps = gPackageDb->GetPackageDependencies(packageInfo.name);
    for (const auto& dep : deps) {
        PackageInfo info;
        if (!gWorkspace.TryGetPackage(dep, info)) {
            continue;
        }

        assert(!dep.empty() && "Dependency name cannot be empty");

        VisitPackage(info);
    }

    ConnectDependencies(packageInfo);
    ConfigurePackage(packageInfo);
    BuildPackage(packageInfo);
    InstallPackage(packageInfo);
    GenerateArtifact(packageInfo.name, packageInfo);
}

static void CheckRequiredTools(PackageDb& db) {
    bool ok = true;
    auto checkTool = [&](const std::string& name) {
        if (db.HasTool(name)) {
            return;
        }

        try {
            if (execute({ name, "--version" }, sp::output{"/dev/null"}, sp::error{"/dev/null"}) != 0) {
                logger.errf("{} is not installed", name);
                ok = false;
            } else {
                logger.logf("{} is installed", name);
            }
        } catch (const std::exception& e) {
            logger.errf("{} is not installed", name);
            ok = false;
        }

        db.AddTool(name);
    };

    checkTool("git");
    checkTool("cmake");
    checkTool("meson");
    checkTool("ninja");
    checkTool("make");
    checkTool("patch");
    // checkTool("fuse-overlayfs");

    if (!ok) {
        exit(1);
    }
}

static void ErasePackageData(const std::string& name) {
    if (fs::exists(PackageBuildPath(name))) {
        fs::remove_all(PackageBuildPath(name));
    }

    if (fs::exists(PackageInstallPath(name))) {
        fs::remove_all(PackageInstallPath(name));
    }

    if (fs::exists(PackageLogPath(name))) {
        fs::remove_all(PackageLogPath(name));
    }

    if (fs::exists(PackageImportPath(name))) {
        fs::remove_all(PackageImportPath(name));
    }
}

static int RunPackageTool(argparse::ArgumentParser& parser) {
    LIBXML_TEST_VERSION;
    defer { xmlCleanupParser(); };

    fs::path build = parser.get<std::string>("--output");
    fs::path prefix = parser.get<std::string>("--prefix");
    fs::path configPath = parser.get<std::string>("--config");
    fs::path targetPath = parser.get<std::string>("--target");

    PackageDb packageDb(build / parser.get<std::string>("--repo"));
    CheckRequiredTools(packageDb);

    gIncludePaths.push_back(configPath.parent_path());
    gIncludePaths.push_back(targetPath.parent_path());

    if (xmlRegisterInputCallbacks(incMatch, incOpen, incRead, incClose) < 0) {
        logger.errf("Failed to register input callbacks");
        return 1;
    }

    XmlDocument targetDocument = XmlDocumentOpen(targetPath);
    if (targetDocument == nullptr) {
        logger.errf("Error: Failed to parse {}", targetPath.string());
        return 1;
    }

    XmlDocument repoDocument = XmlDocumentOpen(configPath);
    if (repoDocument == nullptr) {
        logger.errf("Error: Failed to parse {}", configPath.string());
        return 1;
    }

    XmlNode targetRoot = xmlDocGetRootElement(targetDocument.get());
    XmlNode repoRoot = xmlDocGetRootElement(repoDocument.get());

    gWorkspace.removeOrphans = parser.get<bool>("--remove-orphans");

    auto name = ExpectProperty<std::string>(repoRoot, "name");
    auto sources = ExpectProperty<std::string>(repoRoot, "sources");
    gRepoRoot = configPath.parent_path();
    gBuildRoot = build;
    gInstallPrefix = prefix;
    gSourceRoot = sources;

    MakeFolder(gBuildRoot);
    MakeFolder(PackageCacheRoot());
    MakeFolder(PackageBuildRoot());
    MakeFolder(gInstallPrefix);
    MakeFolder(PackageLogRoot());

    gPackageDb = &packageDb;

    if (parser.present("--workspace")) {
        try {
            gWorkspace.workspace = *argo::parser::load(parser.get<std::string>("--workspace"));
        } catch (const std::exception& e) { }
    } else {
        gWorkspace.workspace = argo::json::json_object();
    }

    gWorkspace.workspace["folders"] = argo::json::json_array();

    gWorkspace.AddFolder(".", "root");

    bool hard = parser.get<bool>("--hard");

    Scope rootScope;

    for (auto define : parser.get<std::vector<std::string>>("--define")) {
        auto parts = stdv::split(define, '=') | stdr::to<std::vector<std::string>>();
        if (parts.size() != 2) {
            throw std::runtime_error("Invalid define format '" + define + "', must be name=value");
        }

        auto name = parts[0];
        auto value = parts[1];

        rootScope.AddVariable(name, value);
    }

    ReadTargetElement(targetRoot, rootScope);

    ReadRepoElement(repoRoot, rootScope);

    auto applyStateLowering = [&](std::string flag, PackageStatus status) {
        auto all = parser.get<std::vector<std::string>>(flag);
        for (const auto& name : all) {
            auto deps = gPackageDb->GetDependantPackages(name);
            for (const auto& dep : deps) {
                PackageInfo info;
                if (!gWorkspace.TryGetPackage(dep, info)) {
                    continue;
                }

                if (hard) {
                    gWorkspace.RelinkPackage(dep);
                }

                gPackageDb->LowerPackageStatus(dep, status);
            }
        }
    };

    applyStateLowering("--reinstall", eBuilt);
    applyStateLowering("--rebuild", eConfigured);
    applyStateLowering("--reconfigure", eDownloaded);
    applyStateLowering("--fetch", eUnknown);

    for (const auto& name : parser.get<std::vector<std::string>>("--fetch")) {
        ErasePackageData(name);
    }

    for (const auto& name : parser.get<std::vector<std::string>>("--clone")) {
        gCloneRepos.insert(name);
    }

    // gPackageDb->DumpTargetStates();

    // Download and extract everything first, we do this now
    // so that we can setup a chroot without it requiring internet
    // access.
    // TODO: actually setup a chroot
    for (auto& package : gWorkspace.packages) {
        AcquirePackage(package.second);
    }

    for (auto& [name, package] : gWorkspace.packages) {
        if (!package.fromSource.empty()) {
            package.imported = fs::absolute(gWorkspace.GetPackagePath(package.fromSource));
        }
    }

    for (auto& pkgName : gPackageDb->GetToplevelPackages()) {
        PackageInfo info;
        if (!gWorkspace.TryGetPackage(pkgName, info)) {
            continue;
        }

        assert(!pkgName.empty() && "Package name cannot be empty");

        VisitPackage(info);
    }

    if (parser.present("--clangd")) {
        GenerateClangDaemonConfig(parser.get<std::vector<std::string>>("--clangd"));
    }

    if (parser.present("--test")) {
        auto tests = parser.get<std::vector<std::string>>("--test");
        for (const auto& test : tests) {
            PackageInfo info;
            if (!gWorkspace.TryGetPackage(test, info)) {
                continue;
            }
            auto path = info.GetBuildFolder().string();
            auto result = execute({ "meson", "test" }, sp::cwd{path});
            if (result != 0) {
                throw std::runtime_error("Failed to run tests for " + test);
            }
        }
    }

    if (parser.present("--workspace")) {
        argo::unparser::save(gWorkspace.workspace, parser.get<std::string>("--workspace"), " ", "\n", " ", 4);
    }

    return 0;
}

#if 0
static void WritePidFile(const fs::path& path) {
    std::ofstream pidfile(path);
    if (!pidfile) {
        throw std::runtime_error("Failed to open PID file for writing");
    }

    pidfile << std::to_string(getpid());
    if (!pidfile) {
        throw std::runtime_error("Failed to write PID file");
    }
}
#endif

static int RunDaemon(argparse::ArgumentParser& parser) {
    return 0;
}

int main(int argc, const char **argv) try {
    OPENSSL_init_crypto(OPENSSL_INIT_ADD_ALL_DIGESTS, nullptr);
    defer { OPENSSL_cleanup(); };

    argparse::ArgumentParser parser{"BezOS package repository manager"};

    parser.add_argument("--config")
        .help("Path to the configuration file")
        .default_value("repo.xml");

    parser.add_argument("--target")
        .help("Path to target file")
        .required();

    parser.add_argument("--workspace")
        .help("Path to vscode workspace file to generate");

    parser.add_argument("--clangd")
        .help("Generate .clangd file for clangd")
        .append()
        .nargs(argparse::nargs_pattern::any);

    parser.add_argument("--output")
        .help("Path to the intermediate build folder")
        .default_value("build");

    parser.add_argument("--fetch")
        .help("List of packages to fetch or update")
        .append()
        .nargs(argparse::nargs_pattern::any);

    parser.add_argument("--clone")
        .help("List of packages to clone from git")
        .append()
        .nargs(argparse::nargs_pattern::any);

    parser.add_argument("--reconfigure")
        .help("List of packages to configure or reconfigure")
        .append()
        .nargs(argparse::nargs_pattern::any);

    parser.add_argument("--reconfigure-hard")
        .help("List of packages to reconfigure from scratch, and reconfigure dependencies")
        .append()
        .nargs(argparse::nargs_pattern::any);

    parser.add_argument("--hard")
        .help("Perform full reconfigures and rebuilds of reverse dependencies")
        .default_value(false)
        .implicit_value(true);

    parser.add_argument("--rebuild")
        .help("List of packages to build or rebuild")
        .append()
        .nargs(argparse::nargs_pattern::any);

    parser.add_argument("--reinstall")
        .help("List of packages to install or reinstall")
        .append()
        .nargs(argparse::nargs_pattern::any);

    parser.add_argument("--prefix")
        .help("Path to the installation prefix")
        .default_value("install");

    parser.add_argument("--repo")
        .help("Name of the repository cache database")
        .default_value("repo.db");

    parser.add_argument("--test")
        .help("List of targets to run tests on")
        .append()
        .nargs(argparse::nargs_pattern::any);

    parser.add_argument("--remove-orphans")
        .help("Remove orphaned packages")
        .default_value(false)
        .implicit_value(true);

    parser.add_argument("--daemon")
        .help("Start the overlayfs support daemon")
        .default_value(false)
        .implicit_value(true);

    parser.add_argument("--define", "-D")
        .help("Define a variable")
        .append()
        .nargs(argparse::nargs_pattern::any);

    parser.add_argument("--verbose")
        .help("Enable verbose output")
        .default_value(false)
        .implicit_value(true);

    parser.add_argument("--help")
        .help("Print this help message")
        .action([&](const std::string &) { std::cout << parser; std::exit(0); });

    try {
        parser.parse_args(argc, argv);
    } catch (const std::runtime_error &e) {
        logger.errf("Error: {}", e.what());
        std::cerr << parser;
        return 1;
    }

    logger.mVerbose = parser.get<bool>("--verbose");
    bool daemon = parser.get<bool>("--daemon");
    if (daemon) {
        return RunDaemon(parser);
    } else {
        return RunPackageTool(parser);
    }
} catch (const std::exception &e) {
    logger.errf("Error: {}", e.what());
    return 1;
}
