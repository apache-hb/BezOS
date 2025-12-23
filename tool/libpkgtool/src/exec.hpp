#pragma once

#include <vector>
#include <string>
#include <ranges>
#include <print>

#include <subprocess.hpp>

namespace pkg {
    template<class... Ts>
    struct overloaded : Ts... {
        using Ts::operator()...;
    };

    template<typename... Args>
    int execute(const std::vector<std::string>& cmd, Args&&... args) {
        std::println("Executing: {}", (cmd | std::views::join_with(' ') | std::ranges::to<std::string>()));
        return subprocess::call(cmd, std::forward<Args>(args)...);
    }
}
