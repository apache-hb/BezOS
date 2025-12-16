#include "pkgtoold/pkgtoold.hpp"
#include <overlay.grpc.pb.h>

#include <csignal>
#include <thread>

#include <grpcpp/server_builder.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include <sys/mount.h>

using namespace bezos::pkgtoold::overlay;

namespace {
std::unique_ptr<grpc::Server> gServer;

class FsOverlayServiceImpl final : public FsOverlayService::Service {
    static constexpr char kOverlayId[] = "overlay";

    grpc::Status CreateOverlay(grpc::ServerContext* context, const CreateOverlayRequest* request, CreateOverlayResponse* response) override {
        std::string overlayPath = request->overlay_path();
        std::string upperdir = request->upper_dir();
        std::string workdir = request->work_dir();
        std::vector<std::string> lowerDirs;
        for (const auto& lowerDir : request->lower_dirs()) {
            lowerDirs.push_back(lowerDir);
        }

        printf("Creating overlay fs: %s\n", request->DebugString().c_str());

        std::stringstream options;
        options << "lowerdir=";
        for (size_t i = 0; i < lowerDirs.size(); ++i) {
            options << lowerDirs[i];
            if (i + 1 < lowerDirs.size()) {
                options << ":";
            }
        }
        options << ",upperdir=" << upperdir << ",workdir=" << workdir;

        int status = mount(kOverlayId, kOverlayId, kOverlayId, 0, options.str().c_str());

        if (status != 0) {
            int err = errno;
            response->set_status(err);
            response->set_detail(std::strerror(err));

            printf("Failed to create overlay fs: %s\n", response->DebugString().c_str());
        } else {
            printf("Overlay fs created successfully at %s\n", overlayPath.c_str());
        }

        return grpc::Status::OK;
    }

    grpc::Status DestroyOverlay(grpc::ServerContext* context, const DestroyOverlayRequest* request, DestroyOverlayResponse* response) override {
        std::string overlayPath = request->overlay_path();
        int status = umount(overlayPath.c_str());

        if (status != 0) {
            int err = errno;
            response->set_status(err);
            response->set_detail(std::strerror(err));
        }

        return grpc::Status::OK;
    }
};

void handleShutdown(int signum) {
    // Use a separate thread to shutdown the server to avoid deadlocks
    std::jthread other([] {
        if (gServer) {
            gServer->Shutdown();
        }
    });
}

bool isInstalled() {
    //
    // TODO: we should really parse /proc/self/status and check the CapEff field
    // instead.
    //
    char path[0x1000];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len == -1) {
        return false;
    }

    path[len] = '\0';
    return std::string_view(path).starts_with("/usr/bin/") || std::string_view(path).starts_with("/bin/");
}

} // namespace

#define LOCALHOST_PATH "localhost:22081"

int main(int argc, const char **argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);

    grpc::EnableDefaultHealthCheckService(true);
    bool installed = isInstalled();
    if (!installed) {
        printf("pkgtoold is not installed system-wide.\n");
    }

    // TODO: add meson support for the reflection plugin
    // grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    FsOverlayServiceImpl service;
    std::string address = installed ? std::format("unix://{}", pkg::pkgtooldUnixSocketPath()) : LOCALHOST_PATH;
    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    gServer = builder.BuildAndStart();
    if (gServer == nullptr) {
        fprintf(stderr, "Failed to start pkgtoold gRPC server\n");
        return 1;
    }

    printf("pkgtoold gRPC server listening on %s\n", address.c_str());

    signal(SIGINT, handleShutdown);
    signal(SIGTERM, handleShutdown);

    gServer->Wait();

    printf("pkgtoold gRPC server shutting down\n");

    return 0;
}
