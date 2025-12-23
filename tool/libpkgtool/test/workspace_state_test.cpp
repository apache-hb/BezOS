#include <gtest/gtest.h>

#include <print>

#include "pkgtool/state.hpp"

namespace fs = std::filesystem;

class WorkspaceStateTest : public testing::Test {

};

TEST_F(WorkspaceStateTest, CreateState) {
    auto tmp = fs::temp_directory_path() / "WorkspaceStateTest_CreateState.db";

    {
        std::println("Creating workspace state at {}", tmp.string());
        auto state = pkg::IWorkspaceState::ofSqlite(tmp);
        ASSERT_NE(state, nullptr);

        auto packageState = state->getPackageState("nonexistent-package");
        ASSERT_EQ(packageState, pkg::PackageState::eUnknown);

        state->addPackage("test-package");
        ASSERT_EQ(state->getPackageState("test-package"), pkg::PackageState::eUnknown);

        state->setPackageState("test-package", pkg::PackageState::eFetched, false);
        ASSERT_EQ(state->getPackageState("test-package"), pkg::PackageState::eFetched);
    }
}
