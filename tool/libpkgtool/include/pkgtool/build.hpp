#pragma once

#include <string>

namespace pkg {
    class ExecuteResult {
        int mResult;
    public:
        ExecuteResult(int result = 0)
            : mResult(result)
        { }

        int getExitCode() const {
            return mResult;
        }
    };

    class ITool {
    public:
        virtual ~ITool() = default;

        virtual std::string name() const = 0;

        virtual ExecuteResult configure() = 0;
        virtual ExecuteResult build() = 0;
        virtual ExecuteResult install() = 0;
        virtual ExecuteResult test() = 0;
    };
}
