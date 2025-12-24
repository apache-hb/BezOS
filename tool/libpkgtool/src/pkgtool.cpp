#include "pkgtool/pkgtool.hpp"
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
public:
    PkgToolImpl(std::shared_ptr<pkg::IWorkspace> workspace)
        : mWorkspace(workspace)
        , mOverlayClient(pkg::IFsOverlayClient::create())
    { }

    std::shared_ptr<pkg::IWorkspace> workspace() const override {
        return mWorkspace;
    }

    void buildPackage(const std::string& name, const std::vector<std::string>& options) override {
        createPackageEnvironment(name);
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

std::shared_ptr<pkg::IPkgTool> pkg::IPkgTool::create(std::shared_ptr<IWorkspace> workspace) {
    return std::make_shared<PkgToolImpl>(workspace);
}
