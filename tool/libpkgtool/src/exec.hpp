#pragma once

#include <vector>
#include <string>
#include <ranges>

#include <quill/Logger.h>
#include <quill/LogMacros.h>

#include <subprocess.hpp>

namespace pkg {
    template<class... Ts>
    struct overloaded : Ts... {
        using Ts::operator()...;
    };

    template<typename... Args>
    int execute(quill::Logger *logger, const std::vector<std::string>& cmd, Args&&... args) {
        LOG_INFO(logger, "Executing: {}", (cmd | std::views::join_with(' ') | std::ranges::to<std::string>()));
        int result = subprocess::call(cmd, std::forward<Args>(args)...);
        LOG_INFO(logger, "Command exited with code {}", result);
        return result;
    }
}
