#pragma once

#include <memory>
#include <vector>
#include <string>

namespace pkg {
    struct CreateOverlayCommand {
        std::string overlayPath;
        std::vector<std::string> lowerDirs;
        std::string upperDir;
        std::string workDir;
    };

    struct DestroyOverlayCommand {
        std::string overlayPath;
    };

    class RpcException : public std::exception {
        int mErrorCode;
        std::string mMessage;
    public:
        RpcException(int err, const std::string& message)
            : mErrorCode(err)
            , mMessage(message)
        { }

        const char* what() const noexcept override {
            return mMessage.c_str();
        }

        int errorCode() const noexcept {
            return mErrorCode;
        }

        std::string_view message() const noexcept {
            return mMessage;
        }
    };

    class IFsOverlayClient {
    public:
        virtual ~IFsOverlayClient() = default;

        virtual void createOverlay(const CreateOverlayCommand& command) = 0;
        virtual void destroyOverlay(const DestroyOverlayCommand& command) = 0;

        static std::shared_ptr<IFsOverlayClient> create();
    };
}
