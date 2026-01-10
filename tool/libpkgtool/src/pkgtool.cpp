#include "pkgtool/build.hpp"
#include "pkgtool/pkgtool.hpp"
#include "pkgtool/state.hpp"

#include "pkgtoold/api.hpp"

#include <quill/Frontend.h>
#include <quill/LogMacros.h>

#include <absl/types/span.h>

#include "pkgtoold/proc_mounts.hpp"
#include "src/exec.hpp"

#include <fmt/format.h>

#include <absl/strings/match.h>

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

    void createOverlayEnvironment(pkg::IPackage& package, absl::Span<std::shared_ptr<pkg::IPackage>> dependencies) {
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

        //
        // Destroy any existing overlay at this path first. This is required
        // if any of the lowerdirs have changed since the last time the overlay
        // was created. (see https://unix.stackexchange.com/questions/588627/how-do-i-merge-directories-read-only-using-overlayfs)
        //
        try {
            mOverlayClient->destroyOverlay({ .overlayPath = overlayCommand.overlayPath });
        } catch (const std::exception& e) {
            // ignore errors
        }

        if (!fs::exists(workdir)) {
            fs::create_directories(workdir);
        }

        mOverlayClient->createOverlay(overlayCommand);
    }

    void createSymlinkEnvironment(pkg::IPackage& package, absl::Span<std::shared_ptr<pkg::IPackage>> dependencies) {
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

    fs::path getSinglePackage(const pkg::DownloadInfo &source, std::shared_ptr<pkg::IPackage> package, bool clone) {
        auto cachePath = pkg::workspaceCachePath(*mWorkspace) / package->name();
        if (clone) {
            if (source.git.empty()) {
                throw std::runtime_error(fmt::format("Package {} source does not specify a git repository to clone", package->name()));
            }

            if (fs::exists(cachePath)) {
                if (fs::exists(cachePath / ".git")) {
                    LOG_TRACE_L1(logger(), "Package '{}' already cloned at '{}', skipping clone", package->name(), cachePath.string());
                    auto pwd = cachePath.string();
                    pkg::execute(logger(), { "git", "restore", "." }, subprocess::cwd{pwd});
                    return cachePath;
                } else {
                    LOG_INFO(logger(), "Package '{}' path '{}' exists but is not a git repository, removing old content", package->name(), cachePath.string());
                    fs::remove_all(cachePath);
                }
            } else {
                fs::create_directories(cachePath);
            }

            auto path = mDownloadClient->clone(source, cachePath);
            LOG_TRACE_L1(logger(), "Cloned source '{}' for package '{}' to '{}'", source.url, package->name(), path.string());

            return cachePath;
        } else {
            if (fs::exists(cachePath)) {
                LOG_INFO(logger(), "Package '{}' path '{}' exists, skipping fetch", package->name(), cachePath.string());
                return cachePath;
            }

            auto path = mDownloadClient->fetch(source);
            LOG_INFO(logger(), "Fetched source '{}' for package '{}' to '{}'", source.url, package->name(), path.string());
            pkg::extractArchive(path, cachePath, source.format, source.trimRootFolder);

            return cachePath;
        }
    }

    void fetchPackageImpl(std::shared_ptr<pkg::IPackage> package, bool clone) {
        auto sources = package->sources();
        if (sources.empty()) {
            LOG_TRACE_L1(logger(), "Package '{}' has no sources, skipping fetch", package->name());
            return;
        }

        auto builddir = pkg::packageBuildPath(*mWorkspace, *package);
        fs::create_directories(builddir);

        LOG_INFO(logger(), "Package {} has {} source(s) to fetch", package->name(), sources.size());

        for (const auto& source : sources) {
            LOG_INFO(logger(), "Fetching source '{}' for package '{}'", source.url, package->name());
            auto dir = getSinglePackage(source, package, clone);

            for (const auto& patch : source.patches) {
                pkg::applyPatch(dir, patch);
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

        if (!fs::exists(package->path() / "builddir")) {
            fs::create_symlink(
                pkg::packageBuildPath(*mWorkspace, *package),
                package->path() / "builddir"
            );
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

        auto installPath = pkg::packageInstallPath(*mWorkspace, *package);
        auto toplevel = mWorkspace->path() / "install" / package->name();
        if (!fs::exists(toplevel)) {
            fs::create_symlink(
                pkg::packageInstallPath(*mWorkspace, *package),
                toplevel
            );
        }
    }
public:
    PkgToolImpl(std::shared_ptr<pkg::IWorkspace> workspace, std::shared_ptr<pkg::IWorkspaceState> state, std::shared_ptr<pkg::IDownloadClient> downloadClient)
        : mWorkspace(workspace)
        , mState(state)
        , mDownloadClient(downloadClient)
        , mOverlayClient(pkg::IFsOverlayClient::create())
    {
        for (const auto& [name, package] : mWorkspace->packages()) {
            mState->addPackage(package->name());
        }

        for (const auto& [name, package] : mWorkspace->packages()) {
            for (const auto& depName : package->publicDependencies()) {
                mState->addDependency(package->name(), depName, pkg::DependencyScope::ePublicDependency | pkg::DependencyScope::ePrivateDependency);
            }

            for (const auto& depName : package->privateDependencies()) {
                mState->addDependency(package->name(), depName, pkg::DependencyScope::ePrivateDependency);
            }
        }
    }

    std::shared_ptr<pkg::IWorkspace> workspace() const override {
        return mWorkspace;
    }

    void fetchPackage(const std::string& name, bool clone) override {
        createPackageEnvironment(name);

        auto package = mWorkspace->package(name);
        if (!package) {
            throw std::runtime_error("Package not found: " + name);
        }

        fetchPackageImpl(package, clone);

        mState->setPackageState(name, pkg::PackageState::eFetched, false);
    }

    void configurePackage(const std::string& name, const std::vector<std::string>& options) override {
        createPackageEnvironment(name);

        auto package = mWorkspace->package(name);
        if (!package) {
            throw std::runtime_error("Package not found: " + name);
        }

        configurePackageImpl(package, options);

        mState->setPackageState(name, pkg::PackageState::eConfigured, false);
    }

    void buildPackage(const std::string& name, const std::vector<std::string>& options) override {
        createPackageEnvironment(name);

        auto package = mWorkspace->package(name);
        if (!package) {
            throw std::runtime_error("Package not found: " + name);
        }

        buildPackageImpl(package, options);

        mState->setPackageState(name, pkg::PackageState::eBuilt, false);
    }

    void installPackage(const std::string& name, const std::vector<std::string>& options) override {
        createPackageEnvironment(name);

        auto package = mWorkspace->package(name);
        if (!package) {
            throw std::runtime_error("Package not found: " + name);
        }

        installPackageImpl(package, options);

        mState->setPackageState(name, pkg::PackageState::eInstalled, false);
    }

    void fetchPackageIfNeeded(const std::string& name, bool clone) override {
        auto state = mState->getPackageState(name);
        if (state >= pkg::PackageState::eFetched) {
            LOG_TRACE_L1(logger(), "Package '{}' is already fetched, skipping", name);
            return;
        }

        fetchPackage(name, clone);

        mState->setPackageState(name, pkg::PackageState::eFetched, false);
    }

    void configurePackageIfNeeded(const std::string& name) override {
        fetchPackageIfNeeded(name, false);

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

        std::vector dependencies = pkg::dependencyClosure(*mWorkspace, package->name());
        if (dependencies.empty()) {
            LOG_TRACE_L1(logger(), "Package '{}' has no dependencies, skipping environment creation", name);
            return;
        }

        if (mOverlayClient->isOverlaySupported()) {
            LOG_TRACE_L1(logger(), "OverlayFS daemon available, creating overlay environment for package '{}'", name);
            createOverlayEnvironment(*package, absl::Span<std::shared_ptr<pkg::IPackage>>{dependencies});
        } else {
            LOG_WARNING_LIMIT(std::chrono::days(1), logger(), "OverlayFS daemon not available, falling back to symlink environment for package '{}'", name);
            createSymlinkEnvironment(*package, absl::Span<std::shared_ptr<pkg::IPackage>>{dependencies});
        }
    }

    void cleanPackageBuildArtifacts(const std::string& name) override {
        auto package = mWorkspace->package(name);
        if (!package) {
            throw std::runtime_error("Package not found: " + name);
        }

        pkg::ProcMounts mounts = pkg::ProcMounts::ofCurrentMachine();
        auto base = pkg::basePackagePath(*mWorkspace, *package);

        for (const auto& entry : mounts.overlayEntries()) {
            if (absl::StartsWith(entry.overlay, base.string())) {
                LOG_INFO(logger(), "Removing overlay mount at '{}' for package '{}'", entry.overlay, name);
                mOverlayClient->destroyOverlay({ .overlayPath = entry.overlay });
            }
        }

        if (fs::exists(base)) {
            fs::remove_all(base);
            LOG_INFO(logger(), "Removed build artifacts at '{}' for package '{}'", base.string(), name);
        } else {
            LOG_TRACE_L1(logger(), "No build artifacts found at '{}' for package '{}', skipping", base.string(), name);
        }

        auto prefix = mWorkspace->path() / "install" / package->name();

        if (fs::exists(prefix)) {
            fs::remove(prefix);
            LOG_INFO(logger(), "Removed install symlink at '{}' for package '{}'", prefix.string(), name);
        }

        auto cache = pkg::packageCachePath(*mWorkspace, *package);
        if (fs::exists(cache)) {
            fs::remove_all(cache);
            LOG_INFO(logger(), "Removed package cache at '{}' for package '{}'", cache.string(), name);
        }
    }

    void lowerPackageState(const std::string& name, pkg::PackageState state) override {
        mState->lowerPackageState(name, state);
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
