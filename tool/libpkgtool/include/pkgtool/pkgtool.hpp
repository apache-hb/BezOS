#pragma once

#include <filesystem>
#include <memory>
#include <vector>
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
        virtual std::string buildTool() const = 0;

        virtual std::filesystem::path path() const = 0;

        virtual std::vector<std::string> buildDependencies() const = 0;
        virtual std::vector<std::string> testDependencies() const = 0;
        virtual std::vector<std::string> dependencies() const = 0;
    };

    /**
     * @brief Get the build path for a package
     * This is the folder where the package is configured and built to.
     *
     * @param package The package
     * @return The build path
     */
    std::filesystem::path packageBuildPath(IPackage& package);

    /**
     * @brief Get the sysroot path for a package
     *
     * This is the folder where the package's sysroot is located.
     *
     * @param package The package
     * @return The sysroot path
     */
    std::filesystem::path packageSysrootPath(IPackage& package);

    /**
     * @brief Get the install path for a package
     *
     * This is the folder where the package installs its build artifacts.
     *
     * @param package The package
     * @return The install path
     */
    std::filesystem::path packageInstallPath(IPackage& package);

    /**
     * @brief Get the private path for a package
     *
     * The private path is used for pkgtool internal data storage.
     *
     * @param package The package
     * @return The private path
     */
    std::filesystem::path packagePrivatePath(IPackage& package);

    class IPkgTool {
    public:
        virtual ~IPkgTool() = default;

        static std::shared_ptr<IPkgTool> create(std::shared_ptr<IWorkspace> workspace);

        virtual std::shared_ptr<IWorkspace> workspace() const = 0;

        virtual void buildPackage(const std::string& name, const std::vector<std::string>& options) = 0;
    };
}
