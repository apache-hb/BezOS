#pragma once

#include <stdexcept>
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

        void throwIfFailed() const {
            if (mResult != 0) {
                throw std::runtime_error("Command failed with exit code " + std::to_string(mResult));
            }
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
