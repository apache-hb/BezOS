#include <gtest/gtest.h>

#include <print>
#include <sstream>

#include "pkgtool/state.hpp"

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

class WorkspaceStateTest : public testing::Test {
protected:
    std::shared_ptr<pkg::IWorkspaceState> mState;
public:
    void SetUp() override {
        auto unitTest = testing::UnitTest::GetInstance();
        auto info = unitTest->current_test_info();
        if (info == nullptr) {
            FAIL() << "Failed to get current test info";
        }

        auto tmp = fs::temp_directory_path() / (std::format("{}_{}.db", info->test_suite_name(), info->name()));
        std::println("Creating workspace state at {}", tmp.string());
        if (fs::exists(tmp)) {
            std::println("Removing existing temporary database at {}", tmp.string());
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

    mState->addDependency("package-a", "package-b", pkg::DependencyScope::eDependency);

    auto buildDeps = mState->getAllDependencies("package-a", pkg::DependencyScope::eBuildDependency);
    ASSERT_TRUE(buildDeps.empty()) << "Expected no build dependencies, got: " << joinStrings(buildDeps, ", ");

    auto testDeps = mState->getAllDependencies("package-a", pkg::DependencyScope::eTestDependency);
    ASSERT_TRUE(testDeps.empty()) << "Expected no test dependencies, got: " << joinStrings(testDeps, ", ");

    auto deps = mState->getAllDependencies("package-a", pkg::DependencyScope::eDependency);
    ASSERT_EQ(deps.size(), 1u) << "Expected one dependency, got: " << joinStrings(deps, ", ");
    ASSERT_EQ(deps.at(0), "package-b");
}

TEST_F(WorkspaceStateTest, AddDependencyMultipleScopes) {
    mState->addPackage("package-a");
    mState->addPackage("package-b");
    mState->addPackage("package-c");

    mState->addDependency("package-a", "package-b", pkg::DependencyScope::eDependency);
    mState->addDependency("package-a", "package-c", pkg::DependencyScope::eBuildDependency);
    mState->addDependency("package-a", "package-c", pkg::DependencyScope::eTestDependency);

    auto deps = mState->getAllDependencies("package-a", pkg::DependencyScope::eDependency);
    ASSERT_EQ(deps.size(), 1u) << "Expected one dependency, got: " << joinStrings(deps, ", ");
    ASSERT_EQ(deps.at(0), "package-b");

    auto buildDeps = mState->getAllDependencies("package-a", pkg::DependencyScope::eBuildDependency);
    ASSERT_EQ(buildDeps.size(), 1u) << "Expected one build dependency, got: " << joinStrings(buildDeps, ", ");
    ASSERT_EQ(buildDeps.at(0), "package-c");

    auto testDeps = mState->getAllDependencies("package-a", pkg::DependencyScope::eTestDependency);
    ASSERT_EQ(testDeps.size(), 1u) << "Expected one test dependency, got: " << joinStrings(testDeps, ", ");
    ASSERT_EQ(testDeps.at(0), "package-c");
}

TEST_F(WorkspaceStateTest, TransitiveDependency) {
    mState->addPackage("package-a");
    mState->addPackage("package-b");
    mState->addPackage("package-c");

    mState->addDependency("package-a", "package-b", pkg::DependencyScope::eDependency);
    mState->addDependency("package-b", "package-c", pkg::DependencyScope::eDependency);

    auto deps = mState->getAllDependencies("package-a", pkg::DependencyScope::eDependency);
    ASSERT_EQ(deps.size(), 2u) << "Expected two dependencies, got: " << joinStrings(deps, ", ");
    ASSERT_EQ(std::find(deps.begin(), deps.end(), "package-b") != deps.end(), true);
    ASSERT_EQ(std::find(deps.begin(), deps.end(), "package-c") != deps.end(), true);
}
