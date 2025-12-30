#include "pkgtool/build.hpp"
#include "pkgtool/pkgtool.hpp"
#include "pkgtool/state.hpp"

#include "pkgtoold/api.hpp"

#include <quill/Frontend.h>
#include <quill/LogMacros.h>

namespace fs = std::filesystem;

namespace {
class PkgToolImpl final : public pkg::IPkgTool {
    static inline auto logger() {
        static auto it = quill::Frontend::create_or_get_logger("PkgToolImpl", quill::Frontend::get_logger("root"));
        return it;
    }

    std::shared_ptr<pkg::IWorkspace> mWorkspace;
    std::shared_ptr<pkg::IWorkspaceState> mState;
    std::shared_ptr<pkg::IDownloadClient> mDownloadClient;
    std::shared_ptr<pkg::IFsOverlayClient> mOverlayClient;

    void createOverlayEnvironment(pkg::IPackage& package, std::span<std::shared_ptr<pkg::IPackage>> dependencies) {
        LOG_TRACE_L1(logger(), "Creating overlay environment for package '{}'", package.name());

        auto sysroot = fs::absolute(pkg::packageSysrootPath(*mWorkspace, package));
        auto installdir = fs::absolute(pkg::packageInstallPath(*mWorkspace, package));
        auto internaldir = fs::absolute(pkg::packagePrivatePath(*mWorkspace, package));
        auto workdir = internaldir / "work";

        pkg::CreateOverlayCommand overlayCommand;
        overlayCommand.overlayPath = sysroot.string();
        overlayCommand.upperDir = installdir.string();
        for (const auto& dependency : dependencies) {
            if (dependency->name() == package.name()) {
                continue;
            }

            auto path = pkg::packageInstallPath(*mWorkspace, *dependency);
            overlayCommand.lowerDirs.push_back(fs::absolute(path).string());
        }
        overlayCommand.workDir = workdir.string();

        mOverlayClient->createOverlay(overlayCommand);
    }

    void createSymlinkEnvironment(pkg::IPackage& package, std::span<std::shared_ptr<pkg::IPackage>> dependencies) {
        LOG_TRACE_L1(logger(), "Creating symlink environment for package '{}'", package.name());

        auto sysroot = fs::absolute(pkg::packageSysrootPath(*mWorkspace, package));
        auto installdir = fs::absolute(pkg::packageInstallPath(*mWorkspace, package));
        auto internaldir = fs::absolute(pkg::packagePrivatePath(*mWorkspace, package));

        for (const auto& dependency : dependencies) {
            auto path = pkg::packageInstallPath(*mWorkspace, *dependency);

            for (const auto& entry : fs::recursive_directory_iterator(fs::absolute(path))) {
                if (entry.is_directory()) {
                    continue;
                }

                auto relativePath = fs::relative(entry.path(), fs::absolute(path));
                auto targetPath = sysroot / relativePath;
                fs::create_directories(targetPath.parent_path());
                fs::create_symlink(entry.path(), targetPath);
            }
        }
    }

    void fetchPackageImpl(std::shared_ptr<pkg::IPackage> package) {
        auto sources = package->sources();
        if (sources.empty()) {
            LOG_TRACE_L1(logger(), "Package '{}' has no sources, skipping fetch", package->name());
            return;
        }

        for (const auto& source : sources) {
            auto path = mDownloadClient->fetch(source);
            LOG_TRACE_L1(logger(), "Fetched source '{}' for package '{}' to '{}'", source.url, package->name(), path.string());
            pkg::extractArchive(path, pkg::workspaceCachePath(*mWorkspace) / package->name(), source.format, source.trimRootFolder);

            for (const auto& patch : source.patches) {
                pkg::applyPatch(pkg::workspaceCachePath(*mWorkspace) / package->name(), patch);
                LOG_TRACE_L1(logger(), "Applied patch '{}' to package '{}'", patch.string(), package->name());
            }
        }
    }

    void configurePackageImpl(std::shared_ptr<pkg::IPackage> package, const std::vector<std::string>& options) {
        auto tool = package->configureTool();
        if (tool == nullptr) {
            LOG_TRACE_L1(logger(), "Package '{}' has no configure tool, skipping", package->name());
            return;
        }

        LOG_TRACE_L1(logger(), "Configuring package '{}' using tool '{}'", package->name(), tool->name());
        tool->configure().throwIfFailed();
    }

    void buildPackageImpl(std::shared_ptr<pkg::IPackage> package, const std::vector<std::string>& options) {
        auto tool = package->buildTool();
        if (tool == nullptr) {
            LOG_TRACE_L1(logger(), "Package '{}' has no build tool, skipping", package->name());
            return;
        }

        LOG_TRACE_L1(logger(), "Building package '{}' using tool '{}'", package->name(), tool->name());
        tool->build().throwIfFailed();
    }

    void installPackageImpl(std::shared_ptr<pkg::IPackage> package, const std::vector<std::string>& options) {
        auto tool = package->installTool();
        if (tool == nullptr) {
            LOG_TRACE_L1(logger(), "Package '{}' has no install tool, skipping", package->name());
            return;
        }

        LOG_TRACE_L1(logger(), "Installing package '{}' using tool '{}'", package->name(), tool->name());
        tool->install().throwIfFailed();
    }
public:
    PkgToolImpl(std::shared_ptr<pkg::IWorkspace> workspace, std::shared_ptr<pkg::IWorkspaceState> state, std::shared_ptr<pkg::IDownloadClient> downloadClient)
        : mWorkspace(workspace)
        , mState(state)
        , mDownloadClient(downloadClient)
        , mOverlayClient(pkg::IFsOverlayClient::create())
    { }

    std::shared_ptr<pkg::IWorkspace> workspace() const override {
        return mWorkspace;
    }

    void fetchPackage(const std::string& name) override {
        createPackageEnvironment(name);

        auto package = mWorkspace->package(name);
        if (!package) {
            throw std::runtime_error("Package not found: " + name);
        }

        fetchPackageImpl(package);
    }

    void configurePackage(const std::string& name, const std::vector<std::string>& options) override {
        createPackageEnvironment(name);

        auto package = mWorkspace->package(name);
        if (!package) {
            throw std::runtime_error("Package not found: " + name);
        }

        configurePackageImpl(package, options);
    }

    void buildPackage(const std::string& name, const std::vector<std::string>& options) override {
        createPackageEnvironment(name);

        auto package = mWorkspace->package(name);
        if (!package) {
            throw std::runtime_error("Package not found: " + name);
        }

        buildPackageImpl(package, options);
    }

    void installPackage(const std::string& name, const std::vector<std::string>& options) override {
        createPackageEnvironment(name);

        auto package = mWorkspace->package(name);
        if (!package) {
            throw std::runtime_error("Package not found: " + name);
        }

        installPackageImpl(package, options);
    }

    void fetchPackageIfNeeded(const std::string& name) override {
        auto state = mState->getPackageState(name);
        if (state >= pkg::PackageState::eFetched) {
            LOG_TRACE_L1(logger(), "Package '{}' is already fetched, skipping", name);
            return;
        }

        fetchPackage(name);

        mState->setPackageState(name, pkg::PackageState::eFetched, false);
    }

    void configurePackageIfNeeded(const std::string& name) override {
        fetchPackageIfNeeded(name);

        auto state = mState->getPackageState(name);
        if (state >= pkg::PackageState::eConfigured) {
            LOG_TRACE_L1(logger(), "Package '{}' is already configured, skipping", name);
            return;
        }

        configurePackage(name, {});

        mState->setPackageState(name, pkg::PackageState::eConfigured, false);
    }

    void buildPackageIfNeeded(const std::string& name) override {
        configurePackageIfNeeded(name);

        auto state = mState->getPackageState(name);
        if (state >= pkg::PackageState::eBuilt) {
            LOG_TRACE_L1(logger(), "Package '{}' is already built, skipping", name);
            return;
        }

        buildPackage(name, {});

        mState->setPackageState(name, pkg::PackageState::eBuilt, false);
    }

    void installPackageIfNeeded(const std::string& name) override {
        buildPackageIfNeeded(name);

        auto state = mState->getPackageState(name);
        if (state >= pkg::PackageState::eInstalled) {
            LOG_TRACE_L1(logger(), "Package '{}' is already installed, skipping", name);
            return;
        }

        installPackage(name, {});

        mState->setPackageState(name, pkg::PackageState::eInstalled, false);
    }

    void createPackageEnvironment(const std::string& name) override {
        auto package = mWorkspace->package(name);
        if (!package) {
            throw std::runtime_error("Package not found: " + name);
        }

        pkg::setupPackageBuildLayout(*mWorkspace, *package);

        std::vector dependencies = pkg::dependencyClosure(*mWorkspace, package->name());
        if (dependencies.empty()) {
            LOG_TRACE_L1(logger(), "Package '{}' has no dependencies, skipping environment creation", name);
            return;
        }

        if (mOverlayClient->isOverlaySupported()) {
            createOverlayEnvironment(*package, dependencies);
        } else {
            LOG_WARNING_LIMIT(std::chrono::days(1), logger(), "OverlayFS daemon not available, falling back to symlink environment for package '{}'", name);
            createSymlinkEnvironment(*package, dependencies);
        }
    }
};
}

std::shared_ptr<pkg::IPkgTool> pkg::IPkgTool::create(
    std::shared_ptr<IWorkspace> workspace,
    std::shared_ptr<IWorkspaceState> state,
    std::shared_ptr<IDownloadClient> downloadClient
) {
    return std::make_shared<PkgToolImpl>(workspace, state, downloadClient);
}
