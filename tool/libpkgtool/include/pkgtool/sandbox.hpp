#pragma once

#include <functional>

namespace pkg {
    enum class SandboxCapability {
        eNone = 0,
        eNetworkAccess = 1 << 0,
        eWritableSourceDir = 1 << 1,
    };

    inline SandboxCapability operator|(SandboxCapability a, SandboxCapability b) {
        return static_cast<SandboxCapability>(static_cast<int>(a) | static_cast<int>(b));
    }

    inline bool hasCapability(SandboxCapability capabilities, SandboxCapability capability) {
        return (static_cast<int>(capabilities) & static_cast<int>(capability)) != 0;
    }

    void runCommandInSandbox(
        SandboxCapability capabilities,
        std::function<void()> run
    );

    template<typename F> requires std::is_invocable_v<F>
    auto runInSandbox(
        SandboxCapability capabilities,
        F&& run
    ) -> decltype(run()) {
        using ReturnType = decltype(run());

        ReturnType result;
        runCommandInSandbox(capabilities, [&] {
            result = run();
        });

        return result;
    }
}
