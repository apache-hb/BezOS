#include "pkgtool/pkgtool.hpp"
#include "pkgtool/state.hpp"
#include "pkgtoold/proc_mounts.hpp"

#include "pkgtoold/api.hpp"

#include <gtest/gtest.h>

#include <filesystem>

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/LogMacros.h>
#include <quill/sinks/ConsoleSink.h>

namespace fs = std::filesystem;

namespace {
quill::Logger *gLogger;

class WorkspaceTest : public testing::Test {
    static inline std::shared_ptr<pkg::IFsOverlayClient> sFsOverlayClient{};
protected:
    fs::path mResourceDir;
public:
    static void SetUpTestSuite() {
        quill::Backend::start();

        quill::ConsoleSinkConfig config;
        config.set_stream("stderr");
        auto console = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("root", config);
        quill::PatternFormatterOptions pattern{"%(time) [%(thread_id)] %(short_source_location:<12) %(log_level:<6) %(message)", "%Y-%m-%dT%H:%M:%S.%QmsZ", quill::Timezone::GmtTime};
        gLogger = quill::Frontend::create_or_get_logger("root", std::move(console), pattern);
        gLogger->set_log_level(quill::LogLevel::TraceL1);

        sFsOverlayClient = pkg::IFsOverlayClient::create();
    }

    static void TearDownTestSuite() {
        gLogger->flush_log();
        quill::Backend::stop();
    }

    void SetUp() override {
        auto unitTest = testing::UnitTest::GetInstance();
        auto info = unitTest->current_test_info();

        //
        // Ideally we'd use a folder in /tmp but overlayfs doesnt work in tmpfs or procfs
        // so we have to use the build directory as our working space here.
        //
        auto tmp = fs::absolute(fs::current_path() / "test_environments" / (std::format("{}_{}", info->test_suite_name(), info->name())));
        LOG_INFO(gLogger, "Copying test resources to {}", tmp.string());
        if (fs::exists(tmp)) {
            LOG_INFO(gLogger, "Removing old resources at {}", tmp.string());
            auto mounts = pkg::ProcMounts::ofCurrentMachine();
            for (const auto& entry : mounts.entries()) {
                LOG_TRACE_L1(gLogger, "Found mount: device='{}' mount='{}' fstype='{}' options='{}'",
                    entry.device,
                    entry.mount,
                    entry.fstype,
                    entry.options);

                if (entry.mount.starts_with(tmp.string())) {
                    LOG_INFO(gLogger, "Unmounting old mount at {}", entry.mount);
                    sFsOverlayClient->destroyOverlay({
                        .overlayPath = entry.mount
                    });
                }
            }
            fs::remove_all(tmp);
        }

        fs::create_directories(tmp);

        auto resourcedir = getenv("RESOURCEDIR");
        ASSERT_NE(resourcedir, nullptr);

        fs::copy(fs::path{resourcedir}, tmp, fs::copy_options::recursive);

        mResourceDir = tmp;
    }
};
}

TEST_F(WorkspaceTest, OpenWorkspace) {
    auto workspace = pkg::IWorkspace::ofRootPath(mResourceDir / "workspace/workspace.xml");
    ASSERT_NE(workspace, nullptr);

    auto packages = workspace->packages();
    ASSERT_EQ(packages.size(), 3u);

    ASSERT_NE(packages.find("package001"), packages.end());
    ASSERT_NE(packages.find("package002"), packages.end());
    ASSERT_NE(packages.find("package003"), packages.end());
}

TEST_F(WorkspaceTest, BuildPackage) {
    auto workspace = pkg::IWorkspace::ofRootPath(mResourceDir / "workspace/workspace.xml");
    ASSERT_NE(workspace, nullptr);

    auto state = pkg::IWorkspaceState::ofSqlite(mResourceDir / "workspace.db");
    ASSERT_NE(state, nullptr);

    auto pkgtool = pkg::IPkgTool::create(workspace, state);

    pkgtool->configurePackageIfNeeded("package001");

    ASSERT_TRUE(fs::exists(mResourceDir / "workspace/build/env/package001/target/install/bin/package001"));
}

TEST_F(WorkspaceTest, BuildDependantPackage) {
    auto workspace = pkg::IWorkspace::ofRootPath(mResourceDir / "workspace/workspace.xml");
    ASSERT_NE(workspace, nullptr);

    auto state = pkg::IWorkspaceState::ofSqlite(mResourceDir / "workspace.db");
    ASSERT_NE(state, nullptr);

    auto pkgtool = pkg::IPkgTool::create(workspace, state);

    pkgtool->configurePackageIfNeeded("package001");
    pkgtool->configurePackageIfNeeded("package002");

    ASSERT_TRUE(fs::exists(mResourceDir / "workspace/build/env/package001/target/install/bin/package001"));
    ASSERT_TRUE(fs::exists(mResourceDir / "workspace/build/env/package002/target/install/bin/package002"));
}
