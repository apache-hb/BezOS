#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace pkg {
    struct DownloadInfo {
        std::string url;
        std::string name;
        std::string sha256Hash;
        std::string format; // "zip", "tar.gz", etc.
        bool trimRootFolder = false;

        std::string git;
        std::string branch;
        std::string commit;

        std::vector<std::filesystem::path> patches;
    };

    class IDownloadClient {
    public:
        virtual ~IDownloadClient() = default;

        static std::shared_ptr<IDownloadClient> create(const std::filesystem::path& cache);

        virtual std::filesystem::path fetch(const DownloadInfo& info) = 0;
        virtual std::filesystem::path clone(const DownloadInfo& info, const std::filesystem::path& dst) = 0;
    };

    void applyPatch(const std::filesystem::path& target, const std::filesystem::path& patch);
    void extractArchive(const std::filesystem::path& archive, const std::filesystem::path& dst, const std::string& format, bool trimRootFolder);
}
