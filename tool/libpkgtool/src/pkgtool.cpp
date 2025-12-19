#include "pkgtool/pkgtool.hpp"
#include "pkgtoold/api.hpp"

#include <print>

namespace fs = std::filesystem;

namespace {
class PkgToolImpl final : public pkg::IPkgTool {
    std::shared_ptr<pkg::IWorkspace> mWorkspace;
    std::shared_ptr<pkg::IFsOverlayClient> mOverlayClient;

    void setupPackageFilesystem(pkg::IPackage& package) {
        auto sysroot = fs::absolute(pkg::packageSysrootPath(*mWorkspace, package));
        auto installdir = fs::absolute(pkg::packageInstallPath(*mWorkspace, package));
        auto internaldir = fs::absolute(pkg::packagePrivatePath(*mWorkspace, package));
        auto workdir = internaldir / "work";

        fs::create_directories(sysroot);
        fs::create_directories(installdir);
        fs::create_directories(internaldir);
        fs::create_directories(workdir);
    }

    void createOverlayEnvironment(pkg::IPackage& package) {
        auto sysroot = fs::absolute(pkg::packageSysrootPath(*mWorkspace, package));
        auto installdir = fs::absolute(pkg::packageInstallPath(*mWorkspace, package));
        auto internaldir = fs::absolute(pkg::packagePrivatePath(*mWorkspace, package));
        auto workdir = internaldir / "work";

        std::vector dependencies = pkg::dependencyClosure(*mWorkspace, package.name());

        pkg::CreateOverlayCommand overlayCommand;
        overlayCommand.overlayPath = sysroot.string();
        overlayCommand.upperDir = installdir.string();
        for (const auto& dependency : dependencies) {
            auto path = pkg::packageInstallPath(*mWorkspace, *dependency);
            overlayCommand.lowerDirs.push_back(fs::absolute(path).string());
        }
        overlayCommand.workDir = workdir.string();

        mOverlayClient->createOverlay(overlayCommand);
    }

    void createSymlinkEnvironment(pkg::IPackage& package) {
        auto sysroot = fs::absolute(pkg::packageSysrootPath(*mWorkspace, package));
        auto installdir = fs::absolute(pkg::packageInstallPath(*mWorkspace, package));
        auto internaldir = fs::absolute(pkg::packagePrivatePath(*mWorkspace, package));
        auto workdir = internaldir / "work";

        std::vector dependencies = pkg::dependencyClosure(*mWorkspace, package.name());

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
    PkgToolImpl(std::shared_ptr<pkg::IWorkspace> workspace, std::shared_ptr<pkg::IFsOverlayClient> overlayClient)
        : mWorkspace(workspace)
        , mOverlayClient(overlayClient)
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

        setupPackageFilesystem(*package);

        if (mOverlayClient->isOverlaySupported()) {
            createOverlayEnvironment(*package);
        } else {
            std::println("[WARN] OverlayFS daemon not available, falling back to symlink environment for package '{}'", name);
            createSymlinkEnvironment(*package);
        }
    }
};
}

std::shared_ptr<pkg::IPkgTool> pkg::IPkgTool::create(std::shared_ptr<IWorkspace> workspace) {
    auto overlayClient = pkg::IFsOverlayClient::create();
    return std::make_shared<PkgToolImpl>(workspace, overlayClient);
}
