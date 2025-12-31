#pragma once

#include <vector>
#include <string>

#include <quill/Logger.h>
#include <quill/LogMacros.h>

#include <subprocess.hpp>

namespace pkg {
    template<class... Ts>
    struct overloaded : Ts... {
        using Ts::operator()...;
    };

    inline std::string joinArgs(const std::vector<std::string>& args) {
        std::stringstream ss;
        for (size_t i = 0; i < args.size(); ++i) {
            ss << args[i];
            if (i + 1 < args.size()) {
                ss << " ";
            }
        }
        return ss.str();
    }

    template<typename... Args>
    int execute(quill::Logger *logger, const std::vector<std::string>& cmd, Args&&... args) {
        LOG_INFO(logger, "Executing: {}", joinArgs(cmd));
        int result = subprocess::call(cmd, std::forward<Args>(args)...);
        LOG_INFO(logger, "Command exited with code {}", result);
        return result;
    }
}
