#include "pkgtoold/pkgtoold.hpp"
#include <expected>
#include <overlay.grpc.pb.h>

#include <csignal>
#include <thread>

#include <grpcpp/server_builder.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include <SQLiteCpp/SQLiteCpp.h>

#include <sys/mount.h>

using namespace bezos::pkgtoold::overlay;

namespace fs = std::filesystem;
namespace sqlite = SQLite;

namespace {

struct PosixError {
    int code;
    std::string message;

    static PosixError ofErrno(int err) {
        return PosixError{err, std::strerror(err)};
    }

    static PosixError success() {
        return PosixError{0, "Success"};
    }

    bool isSuccess() const {
        return code == 0;
    }
};

class Overlay {
    std::string mOverlay;
    std::string mUpper;
    std::string mWork;
    std::vector<std::string> mLowers;

public:
    Overlay() = default;

    std::string getOverlayPath() const {
        return mOverlay;
    }

    std::string getUpperDir() const {
        return mUpper;
    }

    std::string getWorkDir() const {
        return mWork;
    }

    const std::vector<std::string>& getLowerDirs() const {
        return mLowers;
    }

    static std::expected<Overlay, PosixError> create(
        const std::string& overlay,
        const std::string& upper,
        const std::string& work,
        const std::vector<std::string>& lowers
    ) {
        if (!fs::path(overlay).is_absolute()) {
            return std::unexpected(PosixError{EINVAL, "Overlay path must be absolute"});
        }

        if (!fs::path(upper).is_absolute()) {
            return std::unexpected(PosixError{EINVAL, "Upper dir path must be absolute"});
        }

        if (!fs::path(work).is_absolute()) {
            return std::unexpected(PosixError{EINVAL, "Work dir path must be absolute"});
        }

        for (const auto& lower : lowers) {
            if (!fs::path(lower).is_absolute()) {
                return std::unexpected(PosixError{EINVAL, "Lower dir path must be absolute"});
            }
        }

        Overlay o;
        o.mOverlay = overlay;
        o.mUpper = upper;
        o.mWork = work;
        o.mLowers = lowers;
        return o;
    }

    static std::expected<Overlay, PosixError> ofGrpcRequest(const CreateOverlayRequest& request) {
        std::vector<std::string> lowers;
        for (const auto& lower : request.lower_dirs()) {
            lowers.push_back(lower);
        }

        return create(
            request.overlay_path(),
            request.upper_dir(),
            request.work_dir(),
            lowers
        );
    }
};

class OverlayManager final {
    static constexpr char kOverlayId[] = "overlay";
public:
    PosixError createOverlay(const Overlay& overlay) {
        std::stringstream options;
        options << "lowerdir=";
        for (size_t i = 0; i < overlay.getLowerDirs().size(); ++i) {
            options << overlay.getLowerDirs()[i];
            if (i + 1 < overlay.getLowerDirs().size()) {
                options << ":";
            }
        }
        options << ",upperdir=" << overlay.getUpperDir() << ",workdir=" << overlay.getWorkDir();

        int status = mount(kOverlayId, overlay.getOverlayPath().c_str(), kOverlayId, 0, options.str().c_str());

        if (status != 0) {
            return PosixError::ofErrno(errno);
        }

        return PosixError::success();
    }

    PosixError destroyOverlay(const std::string& overlay) {
        int status = umount(overlay.c_str());

        if (status != 0) {
            return PosixError::ofErrno(errno);
        }

        return PosixError::success();
    }
};

class OverlayStorage final {
    sqlite::Database mStorage;

    static constexpr char kSchema[] = R"sql(
        CREATE TABLE IF NOT EXISTS overlays (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            overlay_path TEXT NOT NULL,
            upper_dir TEXT NOT NULL,
            work_dir TEXT NOT NULL,

            UNIQUE(overlay_path)
        );

        CREATE INDEX IF NOT EXISTS idx_overlays_path ON overlays(overlay_path);

        CREATE TABLE IF NOT EXISTS lower_dirs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            overlay_id TEXT NOT NULL,
            path TEXT NOT NULL,
            FOREIGN KEY(overlay_id) REFERENCES overlays(id) ON DELETE CASCADE
        );
    )sql";

    static constexpr char kInsertOverlay[] = R"sql(
        INSERT INTO overlays (overlay_path, upper_dir, work_dir)
        VALUES (?, ?, ?);
    )sql";

    static constexpr char kInsertLowerDir[] = R"sql(
        INSERT INTO lower_dirs (overlay_id, path)
        VALUES (?, ?);
    )sql";

    static constexpr char kOverlayExists[] = R"sql(
        SELECT 1 FROM overlays WHERE overlay_path = ?;
    )sql";

    static constexpr char kQueryOverlays[] = R"sql(
        SELECT id, overlay_path, upper_dir, work_dir FROM overlays;
    )sql";

    static constexpr char kQueryLowerDirs[] = R"sql(
        SELECT path FROM lower_dirs WHERE overlay_id = ?;
    )sql";

    static constexpr char kDeleteOverlay[] = R"sql(
        DELETE FROM overlays WHERE overlay_path = ?;
    )sql";

public:
    OverlayStorage(const fs::path& path)
        : mStorage(path, sqlite::OPEN_READWRITE | sqlite::OPEN_CREATE | sqlite::OPEN_NOFOLLOW)
    {
        mStorage.exec("PRAGMA foreign_keys = ON;");
        mStorage.exec(kSchema);
    }

    void removeOverlay(const std::string& path) {
        sqlite::Statement query(mStorage, kDeleteOverlay);
        query.bind(1, path);
        query.exec();
    }

    void addOverlay(const Overlay& overlay) {
        mStorage.exec("BEGIN TRANSACTION;");

        sqlite::Statement insertOverlay(mStorage, kInsertOverlay);
        insertOverlay.bind(1, overlay.getOverlayPath());
        insertOverlay.bind(2, overlay.getUpperDir());
        insertOverlay.bind(3, overlay.getWorkDir());
        insertOverlay.exec();

        int overlayId = mStorage.getLastInsertRowid();

        sqlite::Statement insertLowerDir(mStorage, kInsertLowerDir);
        for (const auto& lowerDir : overlay.getLowerDirs()) {
            insertLowerDir.bind(1, overlayId);
            insertLowerDir.bind(2, lowerDir);
            insertLowerDir.exec();
            insertLowerDir.reset();
        }

        mStorage.exec("COMMIT;");
    }

    bool overlayExists(const std::string& overlayPath) {
        sqlite::Statement query(mStorage, kOverlayExists);
        query.bind(1, overlayPath);
        return query.executeStep();
    }

    std::vector<Overlay> getOverlays() {
        std::vector<Overlay> overlays;

        sqlite::Statement queryOverlays(mStorage, kQueryOverlays);
        while (queryOverlays.executeStep()) {
            int overlayId = queryOverlays.getColumn(0).getInt();
            auto overlayPath = queryOverlays.getColumn(1).getString();
            auto upperDir = queryOverlays.getColumn(2).getString();
            auto workDir = queryOverlays.getColumn(3).getString();

            sqlite::Statement queryLowerDirs(mStorage, kQueryLowerDirs);
            queryLowerDirs.bind(1, overlayId);
            std::vector<std::string> lowerDirs;
            while (queryLowerDirs.executeStep()) {
                lowerDirs.emplace_back(queryLowerDirs.getColumn(0).getString());
            }

            auto entry = Overlay::create(overlayPath, upperDir, workDir, lowerDirs);
            if (!entry.has_value()) {
                printf("Warning: invalid overlay entry in storage: %s\n", entry.error().message.c_str());
                continue;
            }

            overlays.emplace_back(std::move(entry.value()));
        }

        return overlays;
    }
};

class FsOverlayServiceImpl final : public FsOverlayService::Service {
    OverlayStorage *mStorage;
    OverlayManager *mManager;

    grpc::Status CreateOverlay(grpc::ServerContext* context, const CreateOverlayRequest* request, CreateOverlayResponse* response) override {
        auto overlay = Overlay::ofGrpcRequest(*request);
        if (!overlay.has_value()) {
            auto err = overlay.error();
            printf("Failed to create overlay from request %s: %s\n", err.message.c_str(), request->DebugString().c_str());
            response->set_status(err.code);
            response->set_detail(err.message);
            return grpc::Status::OK;
        }

        auto value = overlay.value();

        mStorage->removeOverlay(value.getOverlayPath());

        printf("Creating overlay fs: %s\n", request->DebugString().c_str());

        auto err = mManager->createOverlay(value);
        if (!err.isSuccess()) {
            response->set_status(err.code);
            response->set_detail(err.message);

            printf("Failed to create overlay fs: %s\n", response->DebugString().c_str());

            return grpc::Status::OK;
        }

        printf("Overlay fs created successfully at %s\n", value.getOverlayPath().c_str());

        try {
            mStorage->addOverlay(value);
        } catch (const std::exception& ex) {
            printf("Failed to store overlay info in database: %s\n", ex.what());
            printf("Continuing without storing overlay info.\n");
        }

        response->set_status(0);
        response->set_detail("Success");

        return grpc::Status::OK;
    }

    grpc::Status DestroyOverlay(grpc::ServerContext* context, const DestroyOverlayRequest* request, DestroyOverlayResponse* response) override {
        std::string overlayPath = request->overlay_path();
        printf("Destroying overlay fs at %s\n", overlayPath.c_str());

        auto status = mManager->destroyOverlay(overlayPath);
        response->set_status(status.code);
        response->set_detail(status.message);

        if (status.isSuccess()) {
            mStorage->removeOverlay(overlayPath);
            printf("Overlay fs at %s destroyed successfully\n", overlayPath.c_str());
        } else {
            printf("Failed to destroy overlay fs at %s: %s\n", overlayPath.c_str(), status.message.c_str());
        }

        return grpc::Status::OK;
    }

public:
    FsOverlayServiceImpl(OverlayStorage *storage, OverlayManager *manager)
        : mStorage(storage)
        , mManager(manager)
    { }
};

std::unique_ptr<grpc::Server> gServer;

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
#define STORAGE_PATH "/var/lib/pkgtoold/overlays.db"

int main(int argc, const char **argv) try {
    setvbuf(stdout, nullptr, _IONBF, 0);

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    bool installed = isInstalled();
    if (!installed) {
        printf("pkgtoold is not installed system-wide, running over tcp.\n");
    } else {
        printf("pkgtoold is installed system-wide, hydrating existing overlays from storage (" STORAGE_PATH ").\n");
        fs::create_directories("/var/lib/pkgtoold");
    }

    OverlayStorage storage{installed ? STORAGE_PATH : "overlays.db"};
    OverlayManager manager;

    for (const auto& overlay : storage.getOverlays()) {
        printf("Hydrating overlay fs at %s\n", overlay.getOverlayPath().c_str());
        auto err = manager.createOverlay(overlay);
        if (!err.isSuccess()) {
            printf("Failed to hydrate overlay fs at %s: %s\n", overlay.getOverlayPath().c_str(), err.message.c_str());
        } else {
            printf("Overlay fs at %s hydrated successfully\n", overlay.getOverlayPath().c_str());
        }
    }

    std::string address = installed ? std::format("unix://{}", pkg::pkgtooldUnixSocketPath()) : LOCALHOST_PATH;
    FsOverlayServiceImpl service{&storage, &manager};
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
} catch (const std::exception& ex) {
    fprintf(stderr, "Fatal error: %s\n", ex.what());
    return 1;
}
