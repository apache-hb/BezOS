#pragma once

#include "pkgtool/download.hpp"
#include "pkgtool/state.hpp"
#include <filesystem>
#include <memory>
#include <vector>
#include <map>

namespace pkg {
    class IWorkspaceState;
    class IWorkspace;
    class IPackage;
    class ITool;

    class IWorkspace {
    public:
        virtual ~IWorkspace() = default;

        static std::shared_ptr<IWorkspace> ofRootPath(const std::filesystem::path& root);

        virtual std::shared_ptr<IPackage> package(std::string_view name) const = 0;
        virtual std::map<std::string, std::shared_ptr<IPackage>> packages() const = 0;

        virtual std::filesystem::path path() const = 0;
    };

    std::vector<std::shared_ptr<IPackage>> dependencyClosure(IWorkspace& workspace, const std::string& name);
    std::vector<std::shared_ptr<IPackage>> totalDependencyClosure(IWorkspace& workspace, const std::string& name);

    class IPackage {
    public:
        virtual ~IPackage() = default;

        static std::shared_ptr<IPackage> of(const std::filesystem::path& folder, IWorkspace& workspace);

        virtual std::string name() const = 0;

        virtual std::shared_ptr<ITool> configureTool() const = 0;
        virtual std::shared_ptr<ITool> buildTool() const = 0;
        virtual std::shared_ptr<ITool> installTool() const = 0;
        virtual std::shared_ptr<ITool> testTool() const = 0;

        virtual std::filesystem::path path() const = 0;

        virtual std::vector<DownloadInfo> sources() const = 0;

        virtual std::vector<std::string> publicDependencies() const = 0;
        virtual std::vector<std::string> privateDependencies() const = 0;
    };

    std::filesystem::path workspaceCachePath(IWorkspace& workspace);

    /**
     * @brief Get the build path for a package
     * This is the folder where the package is configured and built to.
     *
     * @param package The package
     * @return The build path
     */
    std::filesystem::path packageBuildPath(IWorkspace& workspace, IPackage& package);

    /**
     * @brief Get the sysroot path for a package
     *
     * This is the folder where the package's sysroot is located.
     *
     * @param package The package
     * @return The sysroot path
     */
    std::filesystem::path packageSysrootPath(IWorkspace& workspace, IPackage& package);

    /**
     * @brief Get the install path for a package
     *
     * This is the folder where the package installs its build artifacts.
     *
     * @param package The package
     * @return The install path
     */
    std::filesystem::path packageInstallPath(IWorkspace& workspace, IPackage& package);

    std::filesystem::path packageCachePath(IWorkspace& workspace, IPackage& package);

    /**
     * @brief Get the private path for a package
     *
     * The private path is used for pkgtool internal data storage.
     *
     * @param package The package
     * @return The private path
     */
    std::filesystem::path packagePrivatePath(IWorkspace& workspace, IPackage& package);

    std::string evaluate(const std::string& text, IWorkspace& workspace);
    std::string evaluate(const std::string& text, IWorkspace& workspace, IPackage& package);

    void setupWorkspaceLayout(IWorkspace& workspace);
    void setupPackageBuildLayout(IWorkspace& workspace, IPackage& package);
    void setupWorkspace(IWorkspace& workspace);

    class IPkgTool {
    public:
        virtual ~IPkgTool() = default;

        static std::shared_ptr<IPkgTool> create(
            std::shared_ptr<IWorkspace> workspace,
            std::shared_ptr<IWorkspaceState> state,
            std::shared_ptr<IDownloadClient> downloadClient
        );

        virtual std::shared_ptr<IWorkspace> workspace() const = 0;

        virtual void fetchPackage(const std::string& name, bool clone = false) = 0;
        virtual void configurePackage(const std::string& name, const std::vector<std::string>& options) = 0;
        virtual void buildPackage(const std::string& name, const std::vector<std::string>& options) = 0;
        virtual void installPackage(const std::string& name, const std::vector<std::string>& options) = 0;

        virtual void fetchPackageIfNeeded(const std::string& name, bool clone = false) = 0;
        virtual void configurePackageIfNeeded(const std::string& name) = 0;
        virtual void buildPackageIfNeeded(const std::string& name) = 0;
        virtual void installPackageIfNeeded(const std::string& name) = 0;

        virtual void createPackageEnvironment(const std::string& name) = 0;

        virtual void lowerPackageState(const std::string& name, PackageState state) = 0;
    };
}
