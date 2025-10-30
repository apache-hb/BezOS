#pragma once

#include <filesystem>
#include <memory>

namespace pkg {
    class IWorkspace {
    public:
        virtual ~IWorkspace() = default;

        static std::shared_ptr<IWorkspace> ofRootPath(const std::filesystem::path& root);
    };

    class IPackage {
    public:
        virtual ~IPackage() = default;

        static std::shared_ptr<IPackage> of(const std::filesystem::path& folder);
    };
}
