#include "pkgtool/pkgtool.hpp"
#include "pkgtoold/api.hpp"
#include <set>

namespace fs = std::filesystem;

namespace {
class PkgToolImpl final : public pkg::IPkgTool {
    std::shared_ptr<pkg::IWorkspace> mWorkspace;
    std::shared_ptr<pkg::IFsOverlayClient> mOverlayClient;
public:
    PkgToolImpl(std::shared_ptr<pkg::IWorkspace> workspace, std::shared_ptr<pkg::IFsOverlayClient> overlayClient)
        : mWorkspace(workspace)
        , mOverlayClient(overlayClient)
    { }

    std::shared_ptr<pkg::IWorkspace> workspace() const override {
        return mWorkspace;
    }

    void buildPackage(const std::string& name, const std::vector<std::string>& options) override {
        auto package = mWorkspace->package(name);
        if (!package) {
            throw std::runtime_error("Package not found: " + name);
        }

        auto tool = package->buildTool();
        auto builddir = pkg::packageBuildPath(*package);
        auto sysroot = pkg::packageSysrootPath(*package);
        auto installdir = pkg::packageInstallPath(*package);
        auto internaldir = pkg::packagePrivatePath(*package);

        fs::create_directories(builddir);
        fs::create_directories(sysroot);
        fs::create_directories(installdir);
        fs::create_directories(internaldir);

        std::set<std::string> dependencies;

        auto allPackages = mWorkspace->packages();

        auto gatherDependencies = [&](this auto&& self, const std::shared_ptr<pkg::IPackage>& pkg) -> void {
            for (const auto& depName : pkg->dependencies()) {
                if (!allPackages.contains(depName)) {
                    throw std::runtime_error(std::format("Could not resolve {} for {}", depName, pkg->name()));
                }

                dependencies.insert(depName);
                self(allPackages.at(depName));
            }
        };

        gatherDependencies(package);

        pkg::CreateOverlayCommand overlayCommand;
        overlayCommand.overlayPath = sysroot.string();
        overlayCommand.upperDir = installdir.string();
        for (const auto& depName : dependencies) {
            auto depPackage = allPackages.at(depName);
            overlayCommand.lowerDirs.push_back(pkg::packageInstallPath(*depPackage).string());
        }
        overlayCommand.workDir = (internaldir / "work").string();

        mOverlayClient->createOverlay(overlayCommand);
    }
};
}

std::shared_ptr<pkg::IPkgTool> pkg::IPkgTool::create(std::shared_ptr<IWorkspace> workspace) {
    auto overlayClient = pkg::IFsOverlayClient::create();
    return std::make_shared<PkgToolImpl>(workspace, overlayClient);
}
