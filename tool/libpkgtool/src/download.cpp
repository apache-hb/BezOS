#include "pkgtool/download.hpp"

#include <quill/Frontend.h>
#include <quill/LogMacros.h>

#include <curl/curl.h>

#include "src/exec.hpp"

#include <openssl/sha.h>

#include <archive.h>
#include <archive_entry.h>
#include <utime.h>

#include <fstream>

#include <fmt/format.h>

#include <absl/strings/match.h>

#include "defer.hpp" // must be included last, macro `defer` conflicts with cpp-subprocess

namespace fs = std::filesystem;

namespace {
class DownloadClientImpl final : public pkg::IDownloadClient {
    static inline auto logger() {
        static auto it = quill::Frontend::create_or_get_logger("DownloadClientImpl", quill::Frontend::get_logger("root"));
        return it;
    }

    std::filesystem::path mCache;

    bool download(const std::string& url, const std::string& dst) {
        CURL *curl = curl_easy_init();
        if (curl == nullptr) {
            LOG_ERROR(logger(), "Failed to initialize curl");
            return false;
        }

        defer { curl_easy_cleanup(curl); };

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        LOG_INFO(logger(), "Downloading '{}' to '{}'", url, dst);

        FILE *file = fopen(dst.c_str(), "wb");
        if (file == nullptr) {
            LOG_ERROR(logger(), "Failed to open file '{}' for writing", dst);
            return false;
        }

        defer { fclose(file); };

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

        if (auto res = curl_easy_perform(curl); res != CURLE_OK) {
            LOG_ERROR(logger(), "Failed to download '{}': {}", url, curl_easy_strerror(res) ?: "unknown error");
            return false;
        }

        LOG_INFO(logger(), "Finished downloading {} bytes", fs::file_size(dst));

        return true;
    }

    void verifySha256(const fs::path& path, const std::string& expected) {
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

        defer { munmap(ptr, size); };

        unsigned char buffer[SHA256_DIGEST_LENGTH];
        unsigned char *sha = SHA256((const unsigned char *)ptr, size, buffer);
        if (sha == nullptr) {
            throw std::runtime_error("Failed to calculate sha256 for " + path.string());
        }

        std::string hash;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            hash += fmt::format("{:02x}", buffer[i]);
        }

        if (hash != expected) {
            throw std::runtime_error("Hash mismatch for " + path.string() + ": expected " + expected + " got " + hash);
        }
    }

public:
    DownloadClientImpl(const std::filesystem::path& cache)
        : mCache(cache)
    {
        fs::create_directories(mCache);
    }

    std::filesystem::path fetch(const pkg::DownloadInfo& info) override {
        LOG_INFO(logger(), "Downloading '{}' to cache as '{}'", info.url, info.name);

        auto dst = mCache / info.name;

        if (fs::exists(dst) && !info.sha256Hash.empty()) {
            try {
                verifySha256(dst, info.sha256Hash);
                LOG_INFO(logger(), "Using cached file '{}'", dst.string());
                return dst;
            } catch (const std::exception& e) {
                LOG_WARNING(logger(), "Cached file '{}' is invalid: {}", dst.string(), e.what());
                fs::remove(dst);
            }
        }

        if (!download(info.url, dst.string())) {
            throw std::runtime_error("Failed to download file from " + info.url);
        }

        verifySha256(dst, info.sha256Hash);

        return dst;
    }

    std::filesystem::path clone(const pkg::DownloadInfo& info, const std::filesystem::path& dst) override {
        std::string dir = fs::absolute(dst).string();

        LOG_INFO(logger(), "Cloning '{}' to '{}'", info.git, dir);

        std::vector<std::string> args = {
            "git", "clone", info.git, dir
        };

        if (!info.branch.empty()) {
            args.push_back("--branch");
            args.push_back(info.branch);
        }

        auto result = pkg::execute(logger(), args);
        if (result != 0) {
            throw std::runtime_error("Failed to clone " + info.git);
        }

        if (!info.commit.empty()) {
            auto result = pkg::execute(logger(), { "git", "checkout", info.commit }, subprocess::cwd{dir});
            if (result != 0) {
                throw std::runtime_error("Failed to checkout commit " + info.commit);
            }
        }

        pkg::execute(logger(), { "git", "restore", "." }, subprocess::cwd{dir});

        return dst;
    }
};

void copyArchiveData(struct archive *a, std::ostream& os) {
    const void *buff;
    size_t size;
    int64_t offset;

    while (archive_read_data_block(a, &buff, &size, &offset) == ARCHIVE_OK) {
        os.write(reinterpret_cast<const char *>(buff), size);
    }
}

void copyArchiveEntryContent(struct archive *a, struct archive_entry *entry, std::ofstream& os) {
    if (archive_entry_size(entry) > 0) {
        copyArchiveData(a, os);
    }
}

static void copyArchiveEntry(struct archive *a, struct archive_entry *entry, const fs::path& dst, const fs::path& entryPath) {
    fs::path path = entryPath;
    fs::path file = dst / path;

    std::ofstream os{file, std::ios::binary};
    if (!os.is_open()) {
        //
        // Hacky retry logic in case we get a slightly deformed archive file
        // that doesn't create parent directories for files.
        //
        if (!fs::exists(file.parent_path())) {
            fs::create_directories(file.parent_path());
        } else {
            throw std::runtime_error(fmt::format("Failed to open file {}", file.string()));
        }

        std::ofstream os2{file, std::ios::binary};
        if (!os2.is_open()) {
            throw std::runtime_error(fmt::format("Failed to open file {}", file.string()));
        }

        copyArchiveEntryContent(a, entry, os2);
    } else {
        copyArchiveEntryContent(a, entry, os);
    }
}

static void extractArchiveImpl(std::string_view name, const fs::path& archive, const fs::path& dst, bool trimRootFolder) {
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

        if (entryPath.empty()) {
            continue;
        }

        if (absl::EndsWith(entryPath, "/")) {
            fs::create_directories(dst / entryPath);
            continue;
        }

        fs::path path = entryPath;
        fs::path file = dst / path;

        copyArchiveEntry(a, entry, dst, entryPath);

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
    }
}
}

std::shared_ptr<pkg::IDownloadClient> pkg::IDownloadClient::create(const std::filesystem::path& cache) {
    return std::make_shared<DownloadClientImpl>(cache);
}

void pkg::applyPatch(const std::filesystem::path& target, const std::filesystem::path& patch) {
    static auto logger = quill::Frontend::create_or_get_logger("ApplyPatch", quill::Frontend::get_logger("root"));

    std::vector<std::string> args = {
        "patch", "-p1", "-i", patch.string()
    };

    auto cwd = target.string();

    LOG_INFO(logger, "Applying patch '{}' to '{}'", patch.string(), target.string());

    auto result = pkg::execute(logger, args, subprocess::cwd{cwd});
    if (result != 0) {
        throw std::runtime_error(fmt::format("Failed to apply patch {} to {}", patch.string(), target.string()));
    }
}

void pkg::extractArchive(const std::filesystem::path& archive, const std::filesystem::path& dst, const std::string& format, bool trimRootFolder) {
    static auto logger = quill::Frontend::create_or_get_logger("ExtractArchive", quill::Frontend::get_logger("root"));

    LOG_INFO(logger, "Extracting archive '{}' to '{}'", archive.string(), dst.string());
    extractArchiveImpl(archive.filename().string(), archive, dst, trimRootFolder);
}
