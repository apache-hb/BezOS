#include "pkgtool/download.hpp"

#include <quill/Frontend.h>
#include <quill/LogMacros.h>

#include <curl/curl.h>

#include "src/exec.hpp"

#include <openssl/sha.h>

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

public:
    DownloadClientImpl(const std::filesystem::path& cache)
        : mCache(cache)
    {
        fs::create_directories(mCache);
    }

    std::filesystem::path fetch(const pkg::DownloadInfo& info) override {
        LOG_INFO(logger(), "Downloading '{}' to cache as '{}'", info.url, info.name);

        auto dst = mCache / info.name;

        if (!download(info.url, dst.string())) {
            throw std::runtime_error("Failed to download file from " + info.url);
        }

        verifySha256(dst, info.sha256Hash);

        return dst;
    }

    std::filesystem::path clone(const pkg::DownloadInfo& info, const std::filesystem::path& dst) override {
        std::string dir = fs::absolute(dst).string();

        std::vector<std::string> args = {
            "git", "clone", info.url, dir
        };

        if (!info.branch.empty()) {
            args.push_back("--branch");
            args.push_back(info.branch);
        }

        auto result = pkg::execute(logger(), args);
        if (result != 0) {
            throw std::runtime_error("Failed to clone " + info.url);
        }

        if (!info.commit.empty()) {
            auto result = pkg::execute(logger(), { "git", "checkout", info.commit }, subprocess::cwd{dir});
            if (result != 0) {
                throw std::runtime_error("Failed to checkout commit " + info.commit);
            }
        }

        return dst;
    }
};
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

    auto result = pkg::execute(logger, args, subprocess::cwd{cwd});
    if (result != 0) {
        throw std::runtime_error(std::format("Failed to apply patch {} to {}", patch.string(), target.string()));
    }
}
