#pragma once

#include <filesystem>
#include <memory>
#include <string>

namespace pkg {
    struct DownloadInfo {
        std::string url;
        std::string name;
        std::string sha256Hash;

        std::string git;
        std::string branch;
        std::string commit;
    };

    class IDownloadClient {
    public:
        virtual ~IDownloadClient() = default;

        static std::shared_ptr<IDownloadClient> create(const std::filesystem::path& cache);

        virtual std::filesystem::path fetch(const DownloadInfo& info) = 0;
        virtual std::filesystem::path clone(const DownloadInfo& info, const std::filesystem::path& dst) = 0;
    };
}
