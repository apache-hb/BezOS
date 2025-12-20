#include "pkgtoold/api.hpp"
#include "pkgtoold/pkgtoold.hpp"

#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include <overlay.grpc.pb.h>

#include <filesystem>

namespace {

namespace fs = std::filesystem;

using namespace bezos::pkgtoold::overlay;

class FsOverlayClientImpl final : public pkg::IFsOverlayClient {
    std::shared_ptr<grpc::Channel> mChannel;
    std::unique_ptr<FsOverlayService::Stub> mStub;
public:
    FsOverlayClientImpl(std::shared_ptr<grpc::Channel> channel)
        : mChannel(channel)
        , mStub(FsOverlayService::NewStub(channel))
    { }

    void createOverlay(const pkg::CreateOverlayCommand& command) override {
        CreateOverlayRequest request;

        request.set_overlay_path(command.overlayPath);
        request.set_upper_dir(command.upperDir);
        request.set_work_dir(command.workDir);
        for (const auto& lowerDir : command.lowerDirs) {
            request.add_lower_dirs(lowerDir);
        }

        CreateOverlayResponse response;

        grpc::ClientContext context;
        grpc::Status status = mStub->CreateOverlay(&context, request, &response);
        if (!status.ok()) {
            throw pkg::RpcException{-1, std::format("FsOverlayService::CreateOverlay status failed: {}", status.error_message())};
        }

        if (int err = response.status()) {
            throw pkg::RpcException{err, std::format("FsOverlayService::CreateOverlay failed: {} ({})", response.detail(), response.status())};
        }
    }

    void destroyOverlay(const pkg::DestroyOverlayCommand& command) override {
        DestroyOverlayRequest request;
        request.set_overlay_path(command.overlayPath);

        DestroyOverlayResponse response;

        grpc::ClientContext context;
        grpc::Status status = mStub->DestroyOverlay(&context, request, &response);
        if (!status.ok()) {
            throw pkg::RpcException{-1, std::format("FsOverlayService::DestroyOverlay failed: {}", status.error_message())};
        }

        if (int err = response.status()) {
            throw pkg::RpcException{err, std::format("FsOverlayService::DestroyOverlay failed: {} ({})", response.detail(), response.status())};
        }
    }

    bool isOverlaySupported() const override {
        return mChannel->GetState(true) != GRPC_CHANNEL_SHUTDOWN;
    }
};

} // namespace

std::shared_ptr<pkg::IFsOverlayClient> pkg::IFsOverlayClient::create() {
    auto uds = pkg::pkgtooldUnixSocketPath();
    std::string target = fs::exists(uds) ? std::format("unix://{}", uds) : "localhost:22081";
    auto channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    return std::make_shared<FsOverlayClientImpl>(channel);
}
