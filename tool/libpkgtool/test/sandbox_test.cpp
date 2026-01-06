#include "pkgtool/sandbox.hpp"

#include <gtest/gtest.h>

#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/LogMacros.h>
#include <quill/sinks/ConsoleSink.h>

namespace fs = std::filesystem;

namespace {
quill::Logger *gLogger;

class SandboxTest : public testing::Test {
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
};
}

TEST_F(SandboxTest, Simple) {
    pkg::Sandbox args {
        .capabilities = pkg::SandboxCapability::eNone
    };

    int i = pkg::runInSandbox(args, []() -> int {
        return 5 * 5;
    });

    ASSERT_EQ(25, i);
}

TEST_F(SandboxTest, Chroot) {
    fs::path usr = "/usr/bin";
    pkg::Sandbox args {
        .chroot = usr,
        .capabilities = pkg::SandboxCapability::eNone,
    };

    bool i = pkg::runInSandbox(args, [&]() -> bool {
        return fs::exists(fs::path{"/yes"});
    });

    ASSERT_TRUE(i) << "Failed to chroot correctly";
}
