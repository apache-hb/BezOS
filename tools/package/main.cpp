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

struct PackageInfo {
    std::string name;
    fs::path source;
    fs::path build;
    fs::path install;
    fs::path cache;

    std::vector<Download> downloads;
    std::vector<Overlay> overlays;
    std::vector<RequirePackage> dependencies;

    std::string GetWorkspaceFolder() const {
        return source.empty() ? build.string() : source.string();
    }
};

struct Workspace {
    std::map<std::string, PackageInfo> packages;

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

static fs::path PackageCachePath(const std::string &name) {
    return PackageCacheRoot() / name;
}

static fs::path PackageBuildPath(const std::string &name) {
    return PackageBuildRoot() / name;
}

static fs::path PackageInstallPath(const std::string &name) {
    return gInstallPrefix / name;
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
            auto depname = ExpectProperty<std::string>(action, "package");
            if (depname == name) {
                throw std::runtime_error("Package " + name + " cannot require itself");
            }

            if (!gWorkspace.packages.contains(depname)) {
                throw std::runtime_error("Package " + name + " requires unknown package " + depname);
            }

            packageInfo.dependencies.push_back(RequirePackage{
                .name = depname,
                .symlink = ExpectProperty<std::string>(action, "symlink")
            });
        } else if (step == "source"sv) {
            if (!packageInfo.source.empty()) {
                throw std::runtime_error("Package " + name + " already has source folder");
            }

            auto src = ExpectProperty<std::string>(action, "path");
            packageInfo.source = gSourceRoot / src;
        } else if (step == "overlay"sv) {
            auto folder = ExpectProperty<std::string>(action, "path");
            packageInfo.overlays.push_back(Overlay{folder});
        }
    }

    gWorkspace.AddPackage(packageInfo);
}

static void AcquirePackage(const PackageInfo& package) {
    for (const auto& download : package.downloads) {
        auto path = package.cache / download.file;

        std::println(std::cout, "{}: download {} -> {}", package.name, download.url, path.string());

        if (!fs::exists(path)) {
            DownloadFile(download.url, path);
        }

        if (!download.archive.empty()) {
            ExtractArchive(package.name, path, package.build, download.trimRootFolder);
        } else {
            auto dst = package.build / download.file;
            std::println(std::cout, "{}: copy {} -> {}", package.name, path.string(), dst.string());

            fs::copy_file(path, dst, fs::copy_options::overwrite_existing);
        }
    }

    for (const auto& overlay : package.overlays) {
        auto src = gSourceRoot / overlay.folder;
        fs::path dst = package.GetWorkspaceFolder();

        std::println(std::cout, "{}: overlay {} -> {}", package.name, src.string(), dst.string());

        fs::copy(src, dst.parent_path(), fs::copy_options::recursive | fs::copy_options::update_existing);
    }
}

static void ConnectDependencies(const PackageInfo& package) {
    for (const auto& dep : package.dependencies) {
        auto dst = package.GetWorkspaceFolder() / dep.symlink;
        auto path = gWorkspace.GetPackagePath(dep.name);

        std::println(std::cout, "{}: symlink {} -> {}", package.name, path.string(), dst.string());

        if (fs::exists(dst) && fs::equivalent(path, dst)) {
            continue;
        }

        fs::create_directories(dst.parent_path());
        fs::create_directory_symlink(path, dst);
    }
}

int main(int argc, const char **argv) try {
    argparse::ArgumentParser parser{"BezOS package repository manager"};

    parser.add_argument("--config")
        .help("Path to the configuration file")
        .default_value("repo.xml");

    parser.add_argument("--workspace")
        .help("Path to vscode workspace file to generate");

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

    auto name = UnwrapOptional(GetProperty(root, "name"), "Missing repo name property");
    auto output = UnwrapOptional(GetProperty(root, "build"), "Missing repo output property");
    auto install = UnwrapOptional(GetProperty(root, "install"), "Missing repo install property");
    auto sources = UnwrapOptional(GetProperty(root, "sources"), "Missing repo sources property");
    auto pwd = std::filesystem::current_path();
    gRepoRoot = configPath.parent_path();
    gBuildRoot = pwd / output;
    gInstallPrefix = pwd / install;
    gSourceRoot = pwd / sources;

    MakeFolder(gBuildRoot);
    MakeFolder(PackageCacheRoot());
    MakeFolder(PackageBuildRoot());
    MakeFolder(gInstallPrefix);

    gWorkspace.workspace["folders"] = argo::json::json_array();

    gWorkspace.AddFolder(std::filesystem::current_path(), "root");

    for (xmlNodePtr child : NodeChildren(root) | stdv::filter(IsXmlNodeOf(XML_ELEMENT_NODE))) {
        if (NodeName(child) == "package"sv) {
            ReadPackageConfig(child);
        }
    }

    for (auto& package : gWorkspace.packages) {
        AcquirePackage(package.second);
    }

    for (auto& package : gWorkspace.packages) {
        ConnectDependencies(package.second);
    }

    if (parser.present("--workspace")) {
        argo::unparser::save(gWorkspace.workspace, parser.get<std::string>("--vscode-workspace"), " ", "\n", " ", 4);
    }

} catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
}
