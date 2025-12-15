#pragma once

#include <filesystem>
#include <memory>
#include <map>

namespace pkg {
    class IWorkspace;
    class IPackage;

    class IWorkspace {
    public:
        virtual ~IWorkspace() = default;

        static std::shared_ptr<IWorkspace> ofRootPath(const std::filesystem::path& root);

        virtual std::shared_ptr<IPackage> package(std::string_view name) const = 0;
        virtual std::map<std::string, std::shared_ptr<IPackage>> packages() const = 0;
    };

    class IPackage {
    public:
        virtual ~IPackage() = default;

        static std::shared_ptr<IPackage> of(const std::filesystem::path& folder);

        virtual std::string name() const = 0;
    };
}
