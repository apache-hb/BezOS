#pragma once

#include <functional>
#include <filesystem>
#include <optional>

namespace pkg {
    enum class SandboxCapability {
        eNone = 0,
        eNetworkAccess = 1 << 0,
    };

    inline SandboxCapability operator|(SandboxCapability a, SandboxCapability b) {
        return static_cast<SandboxCapability>(static_cast<int>(a) | static_cast<int>(b));
    }

    inline bool hasCapability(SandboxCapability capabilities, SandboxCapability capability) {
        return (static_cast<int>(capabilities) & static_cast<int>(capability)) != 0;
    }

    struct Sandbox {
        std::optional<std::filesystem::path> chroot;
        SandboxCapability capabilities;
    };

    void runCommandInSandbox(
        const Sandbox& sandbox,
        std::function<void()> run
    );

    template<typename F> requires std::is_invocable_v<F>
    auto runInSandbox(
        const Sandbox& sandbox,
        F&& run
    ) -> decltype(run()) {
        using ReturnType = decltype(run());

        ReturnType result;
        runCommandInSandbox(sandbox, [&] {
            result = run();
        });

        return result;
    }
}
