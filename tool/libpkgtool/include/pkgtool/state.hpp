#pragma once

#include <filesystem>
#include <memory>
#include <vector>

namespace pkg {
    class IWorkspace;

    enum class PackageState : uint8_t {
        eUnknown,
        eFetched,
        eConfigured,
        eBuilt,
        eInstalled,
    };

    enum class DependencyScope : uint8_t {
        eDependency = (1 << 0),
        eBuildDependency = (1 << 1),
        eTestDependency = (1 << 2),
    };

    constexpr bool testBit(DependencyScope scopes, DependencyScope scope) {
        return (static_cast<int>(scopes) & static_cast<int>(scope)) != 0;
    }

    class IWorkspaceState {
    public:
        virtual ~IWorkspaceState() = default;

        virtual PackageState getPackageState(const std::string& name) const = 0;
        virtual void setPackageState(const std::string& name, PackageState state, bool recursive) = 0;

        virtual void addPackage(const std::string& name) = 0;

        virtual void addDependency(const std::string& package, const std::string& dependency, DependencyScope scope) = 0;

        virtual std::vector<std::string> getReverseDependencies(const std::string& name, DependencyScope scopes) const = 0;

        virtual std::vector<std::string> getAllDependencies(const std::string& name, DependencyScope scopes) const = 0;
        virtual std::vector<std::string> getDirectDependencies(const std::string& name, DependencyScope scopes) const = 0;

        static std::shared_ptr<IWorkspaceState> ofSqlite(const std::filesystem::path& path);
    };
}
