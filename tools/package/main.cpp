#include <argparse/argparse.hpp>

#include <filesystem>
#include <libxml/parser.h>

#include <subprocess.hpp>

#include <curl/curl.h>
#include <archive.h>
#include <archive_entry.h>

#include <argo.hpp>

#include "defer.hpp"
#include "libxml/tree.h"

#include <ranges>
#include <generator>
#include <iostream>
#include <fstream>

using namespace std::literals;

namespace fs = std::filesystem;
namespace sp = subprocess;
namespace stdr = std::ranges;
namespace stdv = std::views;

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
};

enum ConfigureProgram {
    eNone,

    eMeson,
    eCMake,
};

struct ScriptExec {
    fs::path script;
    std::vector<std::string> args;
    std::map<std::string, std::string> env;
};

struct ArtifactInfo {
    std::string name;

    std::vector<RequirePackage> dependencies;
    std::vector<ScriptExec> scripts;
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
    ConfigureProgram configure{eNone};
    std::string version;
    std::string mesonCrossFile;

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
    std::map<std::string, ArtifactInfo> artifacts;

    argo::json workspace{argo::json::object_e};

    void AddPackage(PackageInfo info) {
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
            throw std::runtime_error("Unknown package " + name);
        }

        return packages[name].GetWorkspaceFolder();
    }

    fs::path GetArtifactPath(const std::string& name);
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

fs::path Workspace::GetArtifactPath(const std::string& name) {
    if (!artifacts.contains(name)) {
        throw std::runtime_error("Unknown artifact " + name);
    }

    return PackageInstallPath(name);
}

static std::generator<xmlNodePtr> NodeChildren(xmlNodePtr node) {
    for (xmlNodePtr child = node->children; child != nullptr; child = child->next) {
        co_yield child;
    }
}

static std::optional<std::string> GetProperty(xmlNodePtr node, const char *name) {
    xmlChar *value = xmlGetProp(node, reinterpret_cast<const xmlChar *>(name));
    if (value == nullptr) {
        return std::nullopt;
    }

    defer { xmlFree(value); };

    return reinterpret_cast<const char *>(value);
}

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
static T ExpectProperty(xmlNodePtr node, const char *name) {
    return UnwrapOptional(GetProperty(node, name), "Missing property " + std::string(name));
}

using XmlDocument = std::unique_ptr<xmlDoc, decltype(&xmlFreeDoc)>;

static XmlDocument xmlDocumentOpen(const std::filesystem::path &path) {
    xmlDocPtr document = xmlReadFile(path.c_str(), nullptr, 0);
    if (document == nullptr) {
        throw std::runtime_error("Failed to parse " + path.string());
    }

    return {document, xmlFreeDoc};
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

    struct archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        std::string entryPath = archive_entry_pathname(entry);

        if (trimRootFolder) {
            entryPath = entryPath.substr(entryPath.find('/') + 1);
        }

        if (entryPath.empty()) {
            continue;
        }

        if (entryPath.ends_with('/')) {
            fs::create_directories(dst / entryPath);
            continue;
        }

        fs::path path = entryPath;
        fs::path file = dst / path;

        std::println(std::cout, "{}: extract {} -> {}", name, path.string(), file.string());

        std::ofstream os(file, std::ios::binary);
        if (!os.is_open()) {
            throw std::runtime_error("Failed to open file "s + path.string());
        }

        if (archive_entry_size(entry) > 0) {
            CopyData(a, os);
        }
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

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        throw std::runtime_error("Failed to download " + url + ": " + curl_easy_strerror(res));
    }
}

static bool ReadRequireTag(xmlNodePtr node, const std::string& name, std::vector<RequirePackage>& deps) {
    auto package = ExpectProperty<std::string>(node, "package");
    auto symlink = GetProperty(node, "symlink");

    if (package == name) {
        throw std::runtime_error("Package " + name + " cannot require itself");
    }

    if (!gWorkspace.packages.contains(package)) {
        throw std::runtime_error("Package " + name + " requires unknown package " + package);
    }

    if (!symlink.has_value()) {
        return false;
    }

    deps.push_back(RequirePackage{package, symlink.value()});
    return true;
}

static void ReadPackageConfig(xmlNodePtr node) {
    fs::path path = UnwrapOptional(GetProperty(node, "path"), "Missing package path property");

    XmlDocument package = xmlDocumentOpen(gRepoRoot / path);
    xmlNodePtr root = xmlDocGetRootElement(package.get());
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

    for (xmlNodePtr action : NodeChildren(root) | stdv::filter(IsXmlNodeOf(XML_ELEMENT_NODE))) {
        auto step = NodeName(action);
        if (step == "download"sv) {
            auto url = ExpectProperty<std::string>(action, "url");
            auto file = ExpectProperty<std::string>(action, "file");
            auto archive = GetProperty(action, "archive");
            auto trimRootFolder = GetProperty(action, "trim-root-folder").value_or("false") == "true";

            packageInfo.downloads.push_back(Download{url, file, archive.value_or(""), trimRootFolder});
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
        } else if (step == "configure"sv) {
            auto with = ExpectProperty<std::string>(action, "with");
            if (with == "meson") {
                packageInfo.configure = eMeson;
                for (xmlNodePtr child : NodeChildren(action) | stdv::filter(IsXmlNodeOf(XML_ELEMENT_NODE))) {
                    if (NodeName(child) == "cross-file"sv) {
                        packageInfo.mesonCrossFile = ExpectProperty<std::string>(child, "path");
                    }
                }
            } else if (with == "cmake") {
                packageInfo.configure = eCMake;
            } else {
                throw std::runtime_error("Unknown configure program " + with);
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

static void ReadArtifactConfig(xmlNodePtr node) {
    auto name = ExpectProperty<std::string>(node, "name");

    ArtifactInfo artifactInfo {
        .name = name
    };

    for (xmlNodePtr action : NodeChildren(node) | stdv::filter(IsXmlNodeOf(XML_ELEMENT_NODE))) {
        auto step = NodeName(action);
        if (step == "require"sv) {
            ReadRequireTag(action, name, artifactInfo.dependencies);
        } else if (step == "script"sv) {
            auto script = ExpectProperty<std::string>(action, "path");
            ScriptExec exec {
                .script = script
            };

            for (xmlNodePtr child : NodeChildren(action) | stdv::filter(IsXmlNodeOf(XML_ELEMENT_NODE))) {
                auto type = NodeName(child);
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

    gWorkspace.artifacts.emplace(name, artifactInfo);
}

static void AcquirePackage(const PackageInfo& package) {
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
}

static void ConnectDependencies(const PackageInfo& package) {
    for (const auto& dep : package.dependencies) {
        auto symlink = package.GetWorkspaceFolder() / dep.symlink;
        auto path = gWorkspace.GetPackagePath(dep.name);

        std::println(std::cout, "{}: symlink {} -> {}", package.name, symlink.string(), path.string());

        if (fs::exists(symlink) && fs::equivalent(path, symlink)) {
            continue;
        }

        fs::create_directories(symlink.parent_path());
        fs::create_directory_symlink(path, symlink);
    }
}

static void ConfigurePackage(const PackageInfo& package) {
    if (package.configure == eNone) {
        return;
    }

    if (package.configure == eMeson) {
        std::vector<std::string> args = {
            "meson", "setup", package.build.string(), "--wipe",
            "--prefix", package.install.string(),
        };
        auto cwd = package.GetWorkspaceFolder().string();

        if (!package.mesonCrossFile.empty()) {
            args.push_back("--cross-file");
            std::string crossFile = package.mesonCrossFile;
            ReplaceAll(crossFile, "@SOURCE_ROOT@", cwd);
            args.push_back(crossFile);
        }

        auto result = sp::call(args, sp::cwd{cwd});
        if (result != 0) {
            throw std::runtime_error("Failed to configure package " + package.name);
        }
    } else {
        throw std::runtime_error("Unknown configure program for " + package.name);
    }
}

static void BuildPackage(const PackageInfo& package) {
    if (package.configure == eNone) {
        return;
    }

    if (package.configure == eMeson) {
        std::string builddir = package.build.string();
        auto result = sp::call({ "meson", "install", "--quiet" }, sp::cwd{builddir});
        if (result != 0) {
            throw std::runtime_error("Failed to build package " + package.name);
        }
    } else {
        throw std::runtime_error("Unknown configure program for " + package.name);
    }
}

static void ReplaceArtifactPlaceholders(std::string& str) {
    ReplaceAll(str, "@PREFIX@", gInstallPrefix.string());
    ReplaceAll(str, "@BUILD@", gBuildRoot.string());
    ReplaceAll(str, "@REPO@", gRepoRoot.string());
    ReplaceAll(str, "@SOURCE@", gSourceRoot.string());
}

int main(int argc, const char **argv) try {
    argparse::ArgumentParser parser{"BezOS package repository manager"};

    parser.add_argument("--config")
        .help("Path to the configuration file")
        .default_value("repo.xml");

    parser.add_argument("--workspace")
        .help("Path to vscode workspace file to generate");

    parser.add_argument("--build")
        .help("Path to the intermediate build folder")
        .default_value("build");

    parser.add_argument("--prefix")
        .help("Path to the installation prefix")
        .default_value("install");

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

    XmlDocument document = xmlDocumentOpen(configPath);
    if (document == nullptr) {
        std::println(std::cerr, "Error: Failed to parse {}", configPath.string());
        return 1;
    }

    xmlNodePtr root = xmlDocGetRootElement(document.get());

    fs::path build = parser.get<std::string>("--build");
    fs::path prefix = parser.get<std::string>("--prefix");

    prefix = fs::absolute(prefix);
    build = fs::absolute(build);

    auto name = UnwrapOptional(GetProperty(root, "name"), "Missing repo name property");
    auto sources = UnwrapOptional(GetProperty(root, "sources"), "Missing sources property");
    auto pwd = std::filesystem::current_path();
    gRepoRoot = fs::absolute(configPath.parent_path());
    gBuildRoot = build;
    gInstallPrefix = prefix;
    gSourceRoot = pwd / sources;

    std::println(std::cout, "config: repo={}, build={}, prefix={}, sources={}", gRepoRoot.string(), gBuildRoot.string(), gInstallPrefix.string(), gSourceRoot.string());

    MakeFolder(gBuildRoot);
    MakeFolder(PackageCacheRoot());
    MakeFolder(PackageBuildRoot());
    MakeFolder(gInstallPrefix);

    gWorkspace.workspace["folders"] = argo::json::json_array();

    gWorkspace.AddFolder(std::filesystem::current_path(), "root");

    for (xmlNodePtr child : NodeChildren(root) | stdv::filter(IsXmlNodeOf(XML_ELEMENT_NODE))) {
        auto type = NodeName(child);
        if (type == "package"sv) {
            ReadPackageConfig(child);
        } else if (type == "artifact"sv) {
            ReadArtifactConfig(child);
        }
    }

    for (auto& package : gWorkspace.packages) {
        AcquirePackage(package.second);
    }

    for (auto& package : gWorkspace.packages) {
        ConnectDependencies(package.second);
    }

    for (auto& package : gWorkspace.packages) {
        ConfigurePackage(package.second);
    }

    for (auto& package : gWorkspace.packages) {
        BuildPackage(package.second);
    }

    for (auto& artifact : gWorkspace.artifacts) {
        for (const auto& script : artifact.second.scripts) {
            auto path = script.script.string();

            std::vector<std::string> args = { "/bin/sh", path };
            args.insert(args.end(), script.args.begin(), script.args.end());

            for (std::string& arg : args) {
                ReplaceArtifactPlaceholders(arg);
            }

            std::map env = script.env;
            env["PREFIX"] = gInstallPrefix.string();
            env["BUILD"] = gBuildRoot.string();
            env["REPO"] = gRepoRoot.string();
            env["SOURCE"] = gSourceRoot.string();

            std::println(std::cout, "{}: execute {}", artifact.first, args[1]);
            auto result = sp::call(args, sp::environment{env});
            if (result != 0) {
                throw std::runtime_error("Failed to run script " + args[1]);
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
