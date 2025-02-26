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

#include "defer.hpp"

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

struct RequirePackage {
    std::string name;
    fs::path symlink;
};

struct Overlay {
    std::string folder;
};

struct Download {
    std::string url;
    std::string file;
    std::string archive;
    bool trimRootFolder;
    bool install;
};

enum ConfigureProgram {
    eConfigureNone,

    eMeson,
    eCMake,
    eShell,
    eAutoconf,
};

enum BuildProgram {
    eBuildNone,

    eNinja,
    eMake,
};

struct ScriptExec {
    fs::path script;
    std::vector<std::string> args;
    std::map<std::string, std::string> env;
};

struct PackageInfo {
    std::string name;
    fs::path source;
    fs::path imported;
    fs::path build;
    fs::path install;
    fs::path cache;

    std::vector<Download> downloads;
    std::vector<Overlay> overlays;
    std::vector<RequirePackage> dependencies;
    ConfigureProgram configure{eConfigureNone};
    BuildProgram buildProgram{eBuildNone};
    std::string version;
    std::string mesonCrossFile;
    std::string mesonNativeFile;
    std::vector<ScriptExec> scripts;
    std::map<std::string, std::string> options;
    std::map<std::string, std::string> configureEnv;
    std::vector<fs::path> patches;
    std::string configureSourcePath;
    std::string shellConfigureScript;

    std::string GetConfigureSourcePath() const {
        return configureSourcePath.empty() ? GetWorkspaceFolder().string() : configureSourcePath;
    }

    fs::path GetWorkspaceFolder() const {
        return source.empty() ? imported.string() : source.string();
    }

    fs::path GetBuildFolder() const {
        return build;
    }

    bool HasCompileCommands() const {
        return fs::exists(GetBuildFolder() / "compile_commands.json");
    }
};

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
    }

    void AddDependency(std::string_view package, std::string_view dependency) {
        sql::Statement query(mDatabase, "INSERT OR REPLACE INTO dependencies (package, dependency) VALUES (?, ?)");
        query.bind(1, package.data());
        query.bind(2, dependency.data());
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
        query.bind(1, name.data());

        while (query.executeStep()) {
            result.insert(query.getColumn(0).getString());
        }

        result.insert(std::string(name));

        return result;
    }

    std::set<std::string> GetPackageDependencies(std::string_view name) {
        std::set<std::string> result;

        sql::Statement query(mDatabase, "SELECT dependency FROM dependencies WHERE package = ?");
        query.bind(1, name.data());

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

        auto visit = [&](this auto&& self, const std::string& name) {
            if (stdr::contains(result, name)) {
                return;
            }

            auto dependants = GetPackageDependencies(name);
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

    PackageStatus GetPackageStatus(std::string_view name) {
        sql::Statement query(mDatabase, "SELECT status FROM targets WHERE name = ?");
        query.bind(1, name.data());

        if (query.executeStep()) {
            auto state = query.getColumn(0).getString();
            return GetPackageStatusFromString(state);
        }

        return eUnknown;
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
        auto current = GetPackageStatus(name);
        if (current > status) {
            SetPackageStatus(name, status);
        }
    }

    void RaiseTargetStatus(std::string_view name, PackageStatus status) {
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

struct Workspace {
    std::map<std::string, PackageInfo> packages;

    argo::json workspace{argo::json::object_e};

    void AddPackage(PackageInfo info) {
        if (packages.contains(info.name)) {
            throw std::runtime_error("Duplicate package " + info.name);
        }

        packages.emplace(info.name, info);

        argo::json folder{argo::json::object_e};
        folder["path"] = info.GetWorkspaceFolder();
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

        return packages[name].GetWorkspaceFolder();
    }

    fs::path GetArtifactPath(const std::string& name);

    bool HasBuildTarget(const std::string& name) {
        return packages.contains(name);
    }

    /// @brief Order the packages in the correct build order
    std::vector<PackageInfo> OrderPackages() {
        std::vector<PackageInfo> result;

        std::set<std::string> visited;
        auto visit = [&](this auto&& self, const std::string& name) {
            if (visited.contains(name)) {
                return;
            }

            visited.insert(name);

            auto& package = packages[name];
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
static fs::path gRepoRoot;
static fs::path gBuildRoot;
static fs::path gSourceRoot;
static fs::path gInstallPrefix;

static fs::path PackageCacheRoot() {
    return gBuildRoot / "cache";
}

static fs::path PackageBuildRoot() {
    return gBuildRoot / "packages";
}

static fs::path PackageImportRoot() {
    return gBuildRoot / "sources";
}

static fs::path PackageCachePath(const std::string &name) {
    return PackageCacheRoot() / name;
}

static fs::path PackageBuildPath(const std::string &name) {
    return PackageBuildRoot() / name;
}

static fs::path PackageInstallPath(const std::string &name) {
    return gInstallPrefix / name;
}

static fs::path PackageImportPath(const std::string &name) {
    return PackageImportRoot() / name;
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

    std::optional<std::string> property(const char *name) const {
        xmlChar *value = xmlGetProp(node, reinterpret_cast<const xmlChar *>(name));
        if (value == nullptr) {
            return std::nullopt;
        }

        defer { xmlFree(value); };

        return reinterpret_cast<const char *>(value);
    }

    std::string content() const {
        return reinterpret_cast<const char *>(node->content);
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

static void MakeFolder(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        if (!std::filesystem::create_directories(path)) {
            throw std::runtime_error("Failed to create directory " + path.string());
        }
    }
}

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

    if (xmlXIncludeProcess(doc.get()) <= 0) {
        throw std::runtime_error("Failed to process xinclude in " + path.string());
    }

    return doc;
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
    struct archive *a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);

    if (archive_read_open_filename(a, archive.c_str(), 10240) != ARCHIVE_OK) {
        throw std::runtime_error("Failed to open archive " + archive.string());
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
        indicators::option::PrefixText{std::format("Extracting {}", fname)},
        indicators::option::ForegroundColor{indicators::Color::white},
        indicators::option::FontStyles{std::vector<indicators::FontStyle>{indicators::FontStyle::bold}}
    };

    struct archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        std::string entryPath = archive_entry_pathname(entry);

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

        std::ofstream os(file, std::ios::binary);
        if (!os.is_open()) {
            throw std::runtime_error("Failed to open file "s + path.string());
        }

        if (archive_entry_size(entry) > 0) {
            CopyData(a, os);
        }

        int count = archive_file_count(a);
        bar.set_option(indicators::option::PostfixText{std::format("{}", count)});
    }

    bar.mark_as_completed();
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
        indicators::option::PrefixText{std::format("Downloading {}", url)},
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

        auto bar = static_cast<indicators::ProgressBar *>(clientp);
        bar->set_option(indicators::option::PostfixText{std::format("{} / {}", dlnow, dltotal)});
        return 0;
    });

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to download " + url + ": " + curl_easy_strerror(res));
    }
}

static bool ReadRequireTag(XmlNode node, const std::string& name, std::vector<RequirePackage>& deps) {
    auto package = ExpectProperty<std::string>(node, "package");
    auto symlink = node.property("symlink");

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

static void ReadPackageConfig(XmlNode root) {
    auto name = ExpectProperty<std::string>(root, "name");

    auto cache = PackageCachePath(name);
    auto build = PackageBuildPath(name);
    auto install = PackageInstallPath(name);

    MakeFolder(build);
    MakeFolder(install);
    MakeFolder(cache);

    PackageInfo packageInfo {
        .name = name,
        .build = build,
        .install = install,
        .cache = cache
    };

    for (XmlNode action : root.children() | stdv::filter(IsXmlNodeOf(XML_ELEMENT_NODE))) {
        auto step = NodeName(action);
        if (step == "download"sv) {
            auto url = ExpectProperty<std::string>(action, "url");
            auto file = ExpectProperty<std::string>(action, "file");
            auto archive = action.property("archive");
            auto trimRootFolder = action.property("trim-root-folder").value_or("false") == "true";
            auto install = action.property("install").value_or("false") == "true";

            packageInfo.downloads.push_back(Download{url, file, archive.value_or(""), trimRootFolder, install});
        } else if (step == "patch"sv) {
            auto file = ExpectProperty<std::string>(action, "file");
            packageInfo.patches.push_back(file);
        } else if (step == "require"sv) {
            ReadRequireTag(action, name, packageInfo.dependencies);
        } else if (step == "source"sv) {
            if (!packageInfo.source.empty()) {
                throw std::runtime_error("Package " + name + " already has source folder");
            }

            auto src = ExpectProperty<std::string>(action, "path");
            packageInfo.source = gSourceRoot / src;
        } else if (step == "overlay"sv) {
            auto folder = ExpectProperty<std::string>(action, "path");
            packageInfo.overlays.push_back(Overlay{folder});
        } else if (step == "build") {
            auto with = ExpectProperty<std::string>(action, "with");
            if (with == "ninja") {
                packageInfo.buildProgram = eNinja;
            } else if (with == "make") {
                packageInfo.buildProgram = eMake;
            } else {
                throw std::runtime_error("Unknown build program " + with);
            }
        } else if (step == "configure"sv) {
            auto with = ExpectProperty<std::string>(action, "with");
            if (with == "meson") {
                packageInfo.configure = eMeson;
                for (XmlNode child : action.children() | stdv::filter(IsXmlNodeOf(XML_ELEMENT_NODE))) {
                    if (child.name() == "cross-file"sv) {
                        packageInfo.mesonCrossFile = ExpectProperty<std::string>(child, "path");
                    } else if (child.name() == "native-file"sv) {
                        packageInfo.mesonNativeFile = ExpectProperty<std::string>(child, "path");
                    }
                }
            } else if (with == "cmake") {
                packageInfo.configure = eCMake;
            } else if (with == "shell") {
                packageInfo.configure = eShell;
                packageInfo.shellConfigureScript = action.content();
            } else if (with == "autoconf") {
                packageInfo.configure = eAutoconf;
            } else {
                throw std::runtime_error("Unknown configure program " + with);
            }

            for (XmlNode child : action.children() | stdv::filter(IsXmlNodeOf(XML_ELEMENT_NODE))) {
                if (child.name() == "options"sv) {
                    packageInfo.options = child.properties();
                } else if (child.name() == "source"sv) {
                    packageInfo.configureSourcePath = ExpectProperty<std::string>(child, "path");
                } else if (child.name() == "env"sv) {
                    packageInfo.configureEnv = child.properties();
                }
            }
        }
    }

    if (packageInfo.source.empty()) {
        auto source = PackageImportPath(name);
        MakeFolder(source);
        packageInfo.imported = source;
    }

    gWorkspace.AddPackage(packageInfo);
}

static void ReadArtifactConfig(XmlNode node) {
    auto name = ExpectProperty<std::string>(node, "name");

    PackageInfo artifactInfo {
        .name = name
    };

    for (XmlNode action : node.children() | stdv::filter(IsXmlNodeOf(XML_ELEMENT_NODE))) {
        auto step = action.name();
        if (step == "require"sv) {
            ReadRequireTag(action, name, artifactInfo.dependencies);
        } else if (step == "script"sv) {
            auto script = ExpectProperty<std::string>(action, "path");
            ScriptExec exec {
                .script = script
            };

            for (XmlNode child : action.children() | stdv::filter(IsXmlNodeOf(XML_ELEMENT_NODE))) {
                auto type = child.name();
                if (type == "arg"sv) {
                    exec.args.push_back(ExpectProperty<std::string>(child, "value"));
                } else if (type == "env"sv) {
                    auto key = ExpectProperty<std::string>(child, "key");
                    auto value = ExpectProperty<std::string>(child, "value");
                    exec.env[key] = value;
                }
            }

            artifactInfo.scripts.push_back(exec);
        }
    }

    gWorkspace.packages.emplace(name, artifactInfo);
}

static void ReplacePathPlaceholders(std::string& str) {
    ReplaceAll(str, "@PREFIX@", gInstallPrefix.string());
    ReplaceAll(str, "@BUILD@", gBuildRoot.string());
    ReplaceAll(str, "@REPO@", gRepoRoot.string());
    ReplaceAll(str, "@SOURCE@", gSourceRoot.string());
}

static void ReplacePackagePlaceholders(std::string& str, const PackageInfo& package) {
    ReplacePathPlaceholders(str);
    ReplaceAll(str, "@PACKAGE@", package.name);
    ReplaceAll(str, "@SOURCE_ROOT@", package.GetWorkspaceFolder().string());
    ReplaceAll(str, "@BUILD_ROOT@", package.GetBuildFolder().string());
    ReplaceAll(str, "@INSTALL_ROOT@", package.install.string());
}

static void AcquirePackage(const PackageInfo& package) {
    if (!gPackageDb->ShouldRunStep(package.name, eDownloaded)) {
        return;
    }

    std::println(std::cout, "{}: acquire", package.name);

    for (const auto& download : package.downloads) {
        auto path = package.cache / download.file;

        std::println(std::cout, "{}: download {} -> {}", package.name, download.url, path.string());

        if (!fs::exists(path)) {
            DownloadFile(download.url, path);
        }

        if (!download.archive.empty()) {
            ExtractArchive(package.name, path, package.GetWorkspaceFolder(), download.trimRootFolder);
        } else {
            auto dst = package.GetWorkspaceFolder() / download.file;
            std::println(std::cout, "{}: copy {} -> {}", package.name, path.string(), dst.string());

            fs::copy_file(path, dst, fs::copy_options::overwrite_existing);
        }
    }

    for (const auto& overlay : package.overlays) {
        auto src = gSourceRoot / overlay.folder;
        fs::path dst = package.GetWorkspaceFolder();

        std::println(std::cout, "{}: overlay {} -> {}", package.name, src.string(), dst.string());

        fs::copy(src, dst, fs::copy_options::recursive | fs::copy_options::update_existing);
    }

    for (const auto& patch : package.patches) {
        auto path = patch.string();
        ReplacePackagePlaceholders(path, package);

        std::println(std::cout, "{}: patch {}", package.name, path);

        std::vector<std::string> args = {
            "patch", "-p1", "-i", path
        };

        auto ws = package.GetWorkspaceFolder().string();

        auto result = sp::call(args, sp::cwd{ws});
        if (result != 0) {
            throw std::runtime_error("Failed to apply patch " + patch.string());
        }
    }

    gPackageDb->RaiseTargetStatus(package.name, eDownloaded);
}

static void AddGitignoreSymlink(const fs::path& path) {
    std::fstream file(std::filesystem::current_path() / ".gitignore", std::ios::in | std::ios::out);

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
        auto symlink = package.GetWorkspaceFolder() / dep.symlink;
        auto path = gWorkspace.GetPackagePath(dep.name);

        auto cwd = fs::current_path();
        auto relative = symlink.lexically_relative(cwd);

        AddGitignoreSymlink(relative);

        if (fs::exists(symlink) && fs::equivalent(path, symlink)) {
            continue;
        }

        std::println(std::cout, "{}: symlink {} -> {}", package.name, symlink.string(), path.string());

        fs::create_directories(symlink.parent_path());
        fs::create_directory_symlink(path, symlink);
    }
}

static void ConfigurePackage(const PackageInfo& package) {
    if (package.configure == eConfigureNone) {
        return;
    }

    if (!gPackageDb->ShouldRunStep(package.name, eConfigured)) {
        return;
    }

    auto cwd = package.GetConfigureSourcePath();
    ReplacePackagePlaceholders(cwd, package);
    std::println(std::cout, "{}: configure {}", package.name, cwd);

    if (package.configure == eMeson) {
        std::vector<std::string> args = {
            "meson", "setup", package.build.string(), "--wipe",
            "--prefix", package.install.string(),
        };

        if (!package.mesonCrossFile.empty()) {
            args.push_back("--cross-file");
            std::string crossFile = package.mesonCrossFile;
            ReplacePackagePlaceholders(crossFile, package);
            args.push_back(crossFile);
        }

        if (!package.mesonNativeFile.empty()) {
            args.push_back("--native-file");
            std::string nativeFile = package.mesonNativeFile;
            ReplacePackagePlaceholders(nativeFile, package);
            args.push_back(nativeFile);
        }

        for (auto& [key, value] : package.options) {
            std::string val = value;
            ReplacePackagePlaceholders(val, package);
            args.push_back("-D" + key + "=" + val);
        }

        auto result = sp::call(args, sp::cwd{cwd});
        if (result != 0) {
            throw std::runtime_error("Failed to configure package " + package.name);
        }
    } else if (package.configure == eCMake) {
        auto builddir = package.build.string();
        if (fs::exists(builddir)) {
            fs::remove_all(builddir);
        }

        std::vector<std::string> args = {
            "cmake", "-S", cwd, "-B", builddir,
            "-DCMAKE_INSTALL_PREFIX=" + package.install.string(),
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
            "-G", "Ninja",
            "-DCMAKE_BUILD_TYPE=MinSizeRel",
        };

        for (auto& [key, value] : package.options) {
            std::string val = value;
            ReplacePackagePlaceholders(val, package);
            args.push_back("-D" + key + "=" + val);
        }

        auto result = sp::call(args, sp::cwd{cwd});
        if (result != 0) {
            throw std::runtime_error("Failed to configure package " + package.name);
        }
    } else if (package.configure == eAutoconf) {
        auto builddir = package.build.string();
        if (fs::exists(builddir)) {
            fs::remove_all(builddir);
        }

        fs::create_directories(builddir);

        std::vector<std::string> args = {
            package.GetConfigureSourcePath() + "/configure", "--prefix=" + package.install.string()
        };

        for (auto& [key, value] : package.options) {
            std::string val = value;
            ReplacePackagePlaceholders(val, package);
            args.push_back("--" + key + "=" + val);
        }

        auto result = sp::call(args, sp::cwd{builddir});
        if (result != 0) {
            throw std::runtime_error("Failed to configure package " + package.name);
        }
    } else {
        throw std::runtime_error("Unknown configure program for " + package.name);
    }

    gPackageDb->RaiseTargetStatus(package.name, eConfigured);
}

static void BuildPackage(const PackageInfo& package) {
    if (package.configure == eConfigureNone) {
        return;
    }

    if (!gPackageDb->ShouldRunStep(package.name, eBuilt)) {
        return;
    }

    std::println(std::cout, "{}: build", package.name);
    std::string builddir = package.build.string();

    if (package.configure == eMeson) {
        auto result = sp::call({ "meson", "compile" }, sp::cwd{builddir});
        if (result != 0) {
            throw std::runtime_error("Failed to build package " + package.name);
        }
    } else if (package.configure == eCMake) {
        auto result = sp::call({ "cmake", "--build", builddir }, sp::cwd{builddir});
        if (result != 0) {
            throw std::runtime_error("Failed to build package " + package.name);
        }
    } else if (package.configure == eAutoconf) {
        auto result = sp::call({ "make", "-j", std::to_string(std::thread::hardware_concurrency()) }, sp::cwd{builddir});
        if (result != 0) {
            throw std::runtime_error("Failed to build package " + package.name);
        }
    } else {
        throw std::runtime_error("Unknown build program for " + package.name);
    }

    gPackageDb->RaiseTargetStatus(package.name, eBuilt);
}

static void CopyInstallFiles(const PackageInfo& package) {
    for (const auto& download : package.downloads) {
        if (download.install) {
            auto path = package.GetWorkspaceFolder() / download.file;
            auto dst = package.install / download.file;

            std::println(std::cout, "{}: copy {} -> {}", package.name, path.string(), dst.string());
            fs::copy_file(path, dst, fs::copy_options::overwrite_existing);
        }
    }
}

static void InstallPackage(const PackageInfo& package) {
    if (package.configure == eConfigureNone) {
        CopyInstallFiles(package);
        return;
    }

    if (!gPackageDb->ShouldRunStep(package.name, eInstalled)) {
        return;
    }

    std::println(std::cout, "{}: install", package.name);
    std::string builddir = package.build.string();

    if (package.configure == eMeson) {
        auto result = sp::call({ "meson", "install", "--quiet", "--no-rebuild", "--skip-subprojects" }, sp::cwd{builddir});
        if (result != 0) {
            throw std::runtime_error("Failed to install package " + package.name);
        }
    } else if (package.configure == eCMake) {
        auto result = sp::call({ "cmake", "--install", builddir }, sp::cwd{builddir});
        if (result != 0) {
            throw std::runtime_error("Failed to install package " + package.name);
        }
    } else if (package.configure == eAutoconf) {
        auto result = sp::call({ "make", "install" }, sp::cwd{builddir});
        if (result != 0) {
            throw std::runtime_error("Failed to install package " + package.name);
        }
    } else {
        throw std::runtime_error("Unknown install program for " + package.name);
    }

    CopyInstallFiles(package);

    gPackageDb->RaiseTargetStatus(package.name, eInstalled);
}

static void GenerateArtifact(std::string_view name, const PackageInfo& artifact) {
    if (!gPackageDb->ShouldRunStep(name, eInstalled)) {
        return;
    }

    std::println(std::cout, "{}: generate", artifact.name);

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

        std::println(std::cout, "{}: execute {}", name, args[1]);
        auto result = sp::call(args, sp::environment{env});
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

static void GenerateClangDaemonConfig() {
    std::string text = []{
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
            if (pkg.HasCompileCommands()) {
                auto workspace = pkg.GetWorkspaceFolder().lexically_relative(fs::current_path());
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

static void ReadRepoElement(XmlNode root) {
    for (XmlNode child : root.children() | stdv::filter(IsXmlNodeOf(XML_ELEMENT_NODE))) {
        auto type = child.name();
        if (type == "package"sv) {
            ReadPackageConfig(child);
        } else if (type == "artifact"sv) {
            ReadArtifactConfig(child);
        } else if (type == "collection"sv) {
            ReadRepoElement(child);
        } else {
            std::println(std::cerr, "Unknown tag {}", type);
        }
    }
}

int main(int argc, const char **argv) try {
    argparse::ArgumentParser parser{"BezOS package repository manager"};

    parser.add_argument("--config")
        .help("Path to the configuration file")
        .default_value("repo.xml");

    parser.add_argument("--workspace")
        .help("Path to vscode workspace file to generate");

    parser.add_argument("--clangd")
        .help("Generate .clangd file for clangd")
        .default_value(false)
        .implicit_value(true);

    parser.add_argument("--output")
        .help("Path to the intermediate build folder")
        .default_value("build");

    parser.add_argument("--fetch")
        .help("List of packages to fetch or update")
        .nargs(argparse::nargs_pattern::any);

    parser.add_argument("--reconfigure")
        .help("List of packages to configure or reconfigure")
        .nargs(argparse::nargs_pattern::any);

    parser.add_argument("--rebuild")
        .help("List of packages to build or rebuild")
        .nargs(argparse::nargs_pattern::any);

    parser.add_argument("--reinstall")
        .help("List of packages to install or reinstall")
        .nargs(argparse::nargs_pattern::any);

    parser.add_argument("--prefix")
        .help("Path to the installation prefix")
        .default_value("install");

    parser.add_argument("--repo")
        .help("Name of the repository cache database")
        .default_value("repo.db");

    parser.add_argument("--test")
        .help("List of targets to run tests on")
        .nargs(argparse::nargs_pattern::any);

    parser.add_argument("--help")
        .help("Print this help message")
        .action([&](const std::string &) { std::cout << parser; std::exit(0); });

    try {
        parser.parse_args(argc, argv);
    } catch (const std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    LIBXML_TEST_VERSION;
    defer { xmlCleanupParser(); };

    fs::path configPath = parser.get<std::string>("--config");

    gIncludePaths.push_back(configPath.parent_path());

    if (xmlRegisterInputCallbacks(incMatch, incOpen, incRead, incClose) < 0) {
        std::println(std::cerr, "Failed to register input callbacks");
        return 1;
    }

    XmlDocument document = XmlDocumentOpen(configPath);
    if (document == nullptr) {
        std::println(std::cerr, "Error: Failed to parse {}", configPath.string());
        return 1;
    }

    XmlNode root = xmlDocGetRootElement(document.get());

    fs::path build = parser.get<std::string>("--output");
    fs::path prefix = parser.get<std::string>("--prefix");

    prefix = fs::absolute(prefix);
    build = fs::absolute(build);

    PackageDb packageDb(build / parser.get<std::string>("--repo"));
    gPackageDb = &packageDb;

    auto name = ExpectProperty<std::string>(root, "name");
    auto sources = ExpectProperty<std::string>(root, "sources");
    auto pwd = std::filesystem::current_path();
    gRepoRoot = fs::absolute(configPath.parent_path());
    gBuildRoot = build;
    gInstallPrefix = prefix;
    gSourceRoot = pwd / sources;

    MakeFolder(gBuildRoot);
    MakeFolder(PackageCacheRoot());
    MakeFolder(PackageBuildRoot());
    MakeFolder(gInstallPrefix);

    gWorkspace.workspace["folders"] = argo::json::json_array();

    gWorkspace.AddFolder(std::filesystem::current_path(), "root");

    ReadRepoElement(root);

    auto applyStateLowering = [&](std::string flag, PackageStatus status) {
        auto all = parser.get<std::vector<std::string>>(flag);
        for (const auto& name : all) {
            auto deps = gPackageDb->GetDependantPackages(name);
            for (const auto& dep : deps) {
                gPackageDb->LowerPackageStatus(dep, status);
            }
        }
    };

    applyStateLowering("--reinstall", eBuilt);
    applyStateLowering("--rebuild", eConfigured);
    applyStateLowering("--reconfigure", eDownloaded);
    applyStateLowering("--fetch", eUnknown);

    gPackageDb->DumpTargetStates();

    for (auto& package : gWorkspace.packages) {
        AcquirePackage(package.second);
    }

    auto pkgs = gPackageDb->OrderPackages()
        | stdv::transform([&](const std::string& name) { return gWorkspace.packages[name]; })
        | stdr::to<std::vector>();

    for (auto& package : pkgs) {
        ConnectDependencies(package);
    }

    for (auto& package : pkgs) {
        ConfigurePackage(package);
    }

    if (parser["--clangd"] == true) {
        GenerateClangDaemonConfig();
    }

    for (auto& package : pkgs) {
        BuildPackage(package);
    }

    for (auto& package : pkgs) {
        InstallPackage(package);
    }

    for (auto& artifact : pkgs) {
        GenerateArtifact(artifact.name, artifact);
    }

    if (parser.present("--test")) {
        auto tests = parser.get<std::vector<std::string>>("--test");
        for (const auto& test : tests) {
            auto path = gWorkspace.GetPackagePath(test).string();
            auto result = sp::call({ "meson", "test" }, sp::cwd{path});
            if (result != 0) {
                throw std::runtime_error("Failed to run tests for " + test);
            }
        }
    }

    if (parser.present("--workspace")) {
        argo::unparser::save(gWorkspace.workspace, parser.get<std::string>("--workspace"), " ", "\n", " ", 4);
    }

} catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
}
