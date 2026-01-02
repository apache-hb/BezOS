#include <gtest/gtest.h>

#include <sstream>

#include "pkgtool/state.hpp"

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/LogMacros.h>
#include <quill/sinks/ConsoleSink.h>

#include <fmt/format.h>

namespace fs = std::filesystem;

namespace {
std::string joinStrings(const std::vector<std::string>& strings, const std::string& delimiter) {
    std::stringstream result;
    for (size_t i = 0; i < strings.size(); ++i) {
        result << strings[i];
        if (i + 1 < strings.size()) {
            result << delimiter;
        }
    }
    return result.str();
}
}

quill::Logger *gLogger;

class WorkspaceStateTest : public testing::Test {
protected:
    std::shared_ptr<pkg::IWorkspaceState> mState;
public:
    static void SetUpTestSuite() {
        quill::Backend::start();

        quill::ConsoleSinkConfig config;
        config.set_stream("stderr");
        auto console = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("root", config);
        quill::PatternFormatterOptions pattern{"%(time) [%(thread_id)] %(short_source_location:<12) %(log_level:<6) %(message)", "%Y-%m-%dT%H:%M:%S.%QmsZ", quill::Timezone::GmtTime};
        gLogger = quill::Frontend::create_or_get_logger("root", std::move(console), pattern);
        gLogger->set_log_level(quill::LogLevel::TraceL1);
    }

    static void TearDownTestSuite() {
        gLogger->flush_log();
        quill::Backend::stop();
    }

    void SetUp() override {
        auto unitTest = testing::UnitTest::GetInstance();
        auto info = unitTest->current_test_info();
        auto tmp = fs::temp_directory_path() / (fmt::format("{}_{}.db", info->test_suite_name(), info->name()));
        LOG_INFO(gLogger, "Creating workspace state at {}", tmp.string());
        if (fs::exists(tmp)) {
            LOG_INFO(gLogger, "Removing existing temporary database at {}", tmp.string());
            fs::remove(tmp);
        }

        mState = pkg::IWorkspaceState::ofSqlite(tmp);
        ASSERT_NE(mState, nullptr);
    }

    void TearDown() override {
        mState.reset();
    }
};

TEST_F(WorkspaceStateTest, CreateState) {
    auto packageState = mState->getPackageState("nonexistent-package");
    ASSERT_EQ(packageState, pkg::PackageState::eUnknown);

    mState->addPackage("test-package");
    ASSERT_EQ(mState->getPackageState("test-package"), pkg::PackageState::eUnknown);

    mState->setPackageState("test-package", pkg::PackageState::eFetched, false);
    ASSERT_EQ(mState->getPackageState("test-package"), pkg::PackageState::eFetched);
}

TEST_F(WorkspaceStateTest, AddDependencySingleScope) {
    mState->addPackage("package-a");
    mState->addPackage("package-b");

    mState->addDependency("package-a", "package-b", pkg::DependencyScope::ePublicDependency);

    auto buildDeps = mState->getAllDependencies("package-a", pkg::DependencyScope::ePrivateDependency);
    ASSERT_TRUE(buildDeps.empty()) << "Expected no private dependencies, got: " << joinStrings(buildDeps, ", ");

    auto deps = mState->getAllDependencies("package-a", pkg::DependencyScope::ePublicDependency);
    ASSERT_EQ(deps.size(), 1u) << "Expected one dependency, got: " << joinStrings(deps, ", ");
    ASSERT_EQ(deps.at(0), "package-b");
}

TEST_F(WorkspaceStateTest, AddDependencyMultipleScopes) {
    mState->addPackage("package-a");
    mState->addPackage("package-b");
    mState->addPackage("package-c");

    mState->addDependency("package-a", "package-b", pkg::DependencyScope::ePublicDependency);
    mState->addDependency("package-a", "package-c", pkg::DependencyScope::ePrivateDependency);
    mState->addDependency("package-a", "package-c", pkg::DependencyScope::ePrivateDependency);

    auto deps = mState->getAllDependencies("package-a", pkg::DependencyScope::ePublicDependency);
    ASSERT_EQ(deps.size(), 1u) << "Expected one dependency, got: " << joinStrings(deps, ", ");
    ASSERT_EQ(deps.at(0), "package-b");

    auto buildDeps = mState->getAllDependencies("package-a", pkg::DependencyScope::ePrivateDependency);
    ASSERT_EQ(buildDeps.size(), 1u) << "Expected one private dependency, got: " << joinStrings(buildDeps, ", ");
    ASSERT_EQ(buildDeps.at(0), "package-c");
}

TEST_F(WorkspaceStateTest, TransitiveDependency) {
    mState->addPackage("package-a");
    mState->addPackage("package-b");
    mState->addPackage("package-c");

    mState->addDependency("package-a", "package-b", pkg::DependencyScope::ePublicDependency);
    mState->addDependency("package-b", "package-c", pkg::DependencyScope::ePublicDependency);

    auto deps = mState->getAllDependencies("package-a", pkg::DependencyScope::ePublicDependency);
    ASSERT_EQ(deps.size(), 2u) << "Expected two dependencies, got: " << joinStrings(deps, ", ");
    ASSERT_EQ(std::find(deps.begin(), deps.end(), "package-b") != deps.end(), true);
    ASSERT_EQ(std::find(deps.begin(), deps.end(), "package-c") != deps.end(), true);
}

TEST_F(WorkspaceStateTest, GetReverseDependencies) {
    mState->addPackage("package-a");
    mState->addPackage("package-b");
    mState->addPackage("package-c");

    mState->addDependency("package-a", "package-b", pkg::DependencyScope::ePublicDependency);
    mState->addDependency("package-b", "package-c", pkg::DependencyScope::ePublicDependency);

    auto rev = mState->getReverseDependencies("package-c", pkg::DependencyScope::ePublicDependency);
    ASSERT_EQ(rev.size(), 2u) << "Expected two reverse dependencies, got: " << joinStrings(rev, ", ");
    ASSERT_TRUE(std::find_if(rev.begin(), rev.end(), [](const std::string& name) { return name == "package-b"; }) != rev.end());
    ASSERT_TRUE(std::find_if(rev.begin(), rev.end(), [](const std::string& name) { return name == "package-a"; }) != rev.end());
}

TEST_F(WorkspaceStateTest, GetReverseDependenciesEmpty) {
    mState->addPackage("package-a");
    mState->addPackage("package-b");

    mState->addDependency("package-a", "package-b", pkg::DependencyScope::ePublicDependency);

    auto rev = mState->getReverseDependencies("package-a", pkg::DependencyScope::ePublicDependency);
    ASSERT_TRUE(rev.empty()) << "Expected no reverse dependencies, got: " << joinStrings(rev, ", ");
}

TEST_F(WorkspaceStateTest, GetReverseDependenciesMultipleScopes) {
    mState->addPackage("package-a");
    mState->addPackage("package-b");
    mState->addPackage("package-c");

    mState->addDependency("package-a", "package-c", pkg::DependencyScope::ePublicDependency);
    mState->addDependency("package-b", "package-c", pkg::DependencyScope::ePrivateDependency);

    auto rev = mState->getReverseDependencies("package-c", pkg::DependencyScope::ePublicDependency | pkg::DependencyScope::ePrivateDependency);
    ASSERT_EQ(rev.size(), 2u) << "Expected two reverse dependencies, got: " << joinStrings(rev, ", ");
    ASSERT_TRUE(std::find_if(rev.begin(), rev.end(), [](const std::string& name) { return name == "package-a"; }) != rev.end());
    ASSERT_TRUE(std::find_if(rev.begin(), rev.end(), [](const std::string& name) { return name == "package-b"; }) != rev.end());
}
