#include "pkgtool/build.hpp"

#include "pkgtool/pkgtool.hpp"
#include "src/exec.hpp"
#include "src/xml.hpp"

#include "basic.hpp"

#include <fstream>

#include <quill/Frontend.h>

namespace fs = std::filesystem;

namespace {
class MesonBuildTool final : public pkg::BasicBuildTool {
    static inline auto logger() {
        static auto it = quill::Frontend::create_or_get_logger("MesonBuildTool", quill::Frontend::get_logger("root"));
        return it;
    }

    std::filesystem::path mCrossFile;
    std::filesystem::path mNativeFile;

    pkg::ExecuteResult runMesonCommand(const std::vector<std::string>& args) {
        std::vector<std::string> cmd = {
            "meson"
        };

        cmd.insert(cmd.end(), args.begin(), args.end());

        auto source = sourcePath().string();
        int result = pkg::execute(logger(), cmd, subprocess::environment{environment()}, subprocess::cwd{source});

        return pkg::ExecuteResult{result};
    }

    std::filesystem::path processConfigFile(const std::filesystem::path& path, pkg::IWorkspace& workspace, pkg::IPackage& package) const {
        std::string content = [&]() {
            std::ifstream file{path};
            if (!file.is_open()) {
                throw std::runtime_error("Failed to open meson config file: " + path.string());
            }

            return std::string{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
        }();

        auto processed = pkg::evaluate(content, workspace, package);
        auto cached = cachePath() / "meson" / path.filename();
        if (!fs::exists(cached.parent_path())) {
            fs::create_directories(cached.parent_path());
        }

        std::ofstream out{cached};
        out << processed;
        out.close();

        return cached;
    }

public:
    MesonBuildTool(XmlNode node, pkg::IWorkspace& workspace, pkg::IPackage& package)
        : pkg::BasicBuildTool(node, workspace, package)
        , mCrossFile(pkg::evaluate(node.property("cross-file").value_or(""), workspace, package))
        , mNativeFile(pkg::evaluate(node.property("native-file").value_or(""), workspace, package))
    {
        if (!mCrossFile.empty()) {
            mCrossFile = processConfigFile(mCrossFile, workspace, package);
        }

        if (!mNativeFile.empty()) {
            mNativeFile = processConfigFile(mNativeFile, workspace, package);
        }
    }

    std::string name() const override {
        return "meson";
    }

    pkg::ExecuteResult configure() override {
        std::vector<std::string> cmd = {
            "setup", buildPath().string(), sourcePath().string(),
            "--prefix", installPrefix().string(),
        };

        if (!mCrossFile.empty()) {
            cmd.push_back("--cross-file");
            cmd.push_back(mCrossFile.string());
        }

        if (!mNativeFile.empty()) {
            cmd.push_back("--native-file");
            cmd.push_back(mNativeFile.string());
        }

        for (const auto& [key, value] : options()) {
            cmd.push_back("-D" + key + "=" + value);
        }

        return runMesonCommand(cmd);
    }

    pkg::ExecuteResult build() override {
        std::vector<std::string> cmd = {
            "compile", "-C", buildPath().string()
        };

        return runMesonCommand(cmd);
    }

    pkg::ExecuteResult install() override {
        std::vector<std::string> cmd = {
            "install", "-C", buildPath().string(), "--skip-subprojects"
        };

        return runMesonCommand(cmd);
    }

    pkg::ExecuteResult test() override {
        std::vector<std::string> cmd = {
            "test", "-C", buildPath().string()
        };

        return runMesonCommand(cmd);
    }
};
}

std::shared_ptr<pkg::ITool> pkg::detail::getMesonBuildTool(XmlNode node, IWorkspace& workspace, IPackage& package) {
    return std::make_shared<MesonBuildTool>(node, workspace, package);
}
