#include "pkgtoold/pkgtoold.hpp"

#include <overlay.grpc.pb.h>

#include <csignal>
#include <thread>
#include <expected>

#include <grpcpp/server_builder.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include <SQLiteCpp/SQLiteCpp.h>

#include <sys/mount.h>

#include <quill/SimpleSetup.h>
#include <quill/LogFunctions.h>

using namespace bezos::pkgtoold::overlay;

namespace fs = std::filesystem;
namespace sqlite = SQLite;

namespace {
quill::Logger* gLogger;

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

    static constexpr char kGetOverlay[] = R"sql(
        SELECT id, upper_dir, work_dir FROM overlays WHERE overlay_path = ?;
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
        sqlite::Statement query{mStorage, kDeleteOverlay};
        query.bind(1, path);
        query.exec();
    }

    void addOverlay(const Overlay& overlay) {
        mStorage.exec("BEGIN TRANSACTION;");

        sqlite::Statement insertOverlay{mStorage, kInsertOverlay};
        insertOverlay.bind(1, overlay.getOverlayPath());
        insertOverlay.bind(2, overlay.getUpperDir());
        insertOverlay.bind(3, overlay.getWorkDir());
        insertOverlay.exec();

        int overlayId = mStorage.getLastInsertRowid();

        sqlite::Statement insertLowerDir{mStorage, kInsertLowerDir};
        for (const auto& lowerDir : overlay.getLowerDirs()) {
            insertLowerDir.bind(1, overlayId);
            insertLowerDir.bind(2, lowerDir);
            insertLowerDir.exec();
            insertLowerDir.reset();
        }

        mStorage.exec("COMMIT;");
    }

    bool overlayExists(const std::string& overlayPath) {
        sqlite::Statement query{mStorage, kOverlayExists};
        query.bind(1, overlayPath);
        return query.executeStep();
    }

    std::optional<Overlay> getOverlay(const std::string& overlayPath) {
        sqlite::Statement queryOverlays{mStorage, kGetOverlay};
        queryOverlays.bind(1, overlayPath);

        if (!queryOverlays.executeStep()) {
            quill::info(gLogger, "No overlay fs found at {} in storage.", overlayPath);
            return std::nullopt;
        }

        int overlayId = queryOverlays.getColumn(0).getInt();
        auto upperDir = queryOverlays.getColumn(1).getString();
        auto workDir = queryOverlays.getColumn(2).getString();

        quill::info(gLogger, "Found overlay fs at {} in storage.", overlayPath);

        sqlite::Statement queryLowerDirs{mStorage, kQueryLowerDirs};
        queryLowerDirs.bind(1, overlayId);
        std::vector<std::string> lowerDirs;
        while (queryLowerDirs.executeStep()) {
            lowerDirs.emplace_back(queryLowerDirs.getColumn(0).getString());
        }

        quill::info(gLogger, "Overlay fs at {} has {} lower dirs.", overlayPath, lowerDirs.size());

        auto entry = Overlay::create(overlayPath, upperDir, workDir, lowerDirs);
        if (!entry.has_value()) {
            quill::warning(gLogger, "Invalid overlay entry in storage for path {}: {}", overlayPath, entry.error().message);
            return std::nullopt;
        }

        return entry.value();
    }

    std::vector<Overlay> getOverlays() {
        std::vector<Overlay> overlays;

        sqlite::Statement queryOverlays{mStorage, kQueryOverlays};
        while (queryOverlays.executeStep()) {
            int overlayId = queryOverlays.getColumn(0).getInt();
            auto overlayPath = queryOverlays.getColumn(1).getString();
            auto upperDir = queryOverlays.getColumn(2).getString();
            auto workDir = queryOverlays.getColumn(3).getString();

            sqlite::Statement queryLowerDirs{mStorage, kQueryLowerDirs};
            queryLowerDirs.bind(1, overlayId);
            std::vector<std::string> lowerDirs;
            while (queryLowerDirs.executeStep()) {
                lowerDirs.emplace_back(queryLowerDirs.getColumn(0).getString());
            }

            auto entry = Overlay::create(overlayPath, upperDir, workDir, lowerDirs);
            if (!entry.has_value()) {
                quill::warning(gLogger, "Invalid overlay entry in storage: {}", entry.error().message);
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
            quill::warning(gLogger, "Failed to validate overlay request {}: {}", request->DebugString(), err.message);
            response->set_status(err.code);
            response->set_detail(err.message);
            return grpc::Status::OK;
        }

        auto value = overlay.value();

        mStorage->removeOverlay(value.getOverlayPath());

        quill::info(gLogger, "Creating overlay fs: {}", request->DebugString());

        auto err = mManager->createOverlay(value);
        if (!err.isSuccess()) {
            response->set_status(err.code);
            response->set_detail(err.message);

            quill::warning(gLogger, "Failed to create overlay fs {}: {}", request->DebugString(), err.message);

            return grpc::Status::OK;
        }

        quill::info(gLogger, "Overlay fs created successfully at {}", value.getOverlayPath());

        try {
            mStorage->addOverlay(value);
        } catch (const std::exception& ex) {
            quill::warning(gLogger, "Failed to store overlay info in database: {}. Continuing on without persistence.", ex.what());
        }

        response->set_status(0);
        response->set_detail("Success");

        return grpc::Status::OK;
    }

    grpc::Status destroyOverlayImpl(grpc::ServerContext* context, const DestroyOverlayRequest* request, DestroyOverlayResponse* response) {
        std::string overlayPath = request->overlay_path();
        quill::info(gLogger, "Destroying overlay fs at {}", overlayPath);

        auto overlay = mStorage->getOverlay(overlayPath);
        if (!overlay.has_value()) {
            quill::warning(gLogger, "No overlay fs found at {} in storage.", overlayPath);
            response->set_status(ENOENT);
            response->set_detail("Overlay not found in storage");
            return grpc::Status::OK;
        } else {
            quill::info(gLogger, "Found overlay fs at {} in storage.", overlay->getOverlayPath());
        }

        auto status = mManager->destroyOverlay(overlay->getOverlayPath());
        response->set_status(status.code);
        response->set_detail(status.message);

        quill::info(gLogger, "Overlay fs at {} unmount operation completed with status: {} - {}", overlayPath, status.code, status.message);

        if (status.isSuccess()) {
            mStorage->removeOverlay(overlayPath);
            quill::info(gLogger, "Overlay fs at {} destroyed successfully.", overlayPath);

            try {
                // The workdir is owned by the root uid so we need to remove it, since the client
                // won't have permission to do so.
                fs::remove_all(overlay->getWorkDir());
            } catch (const std::exception& ex) {
                quill::warning(gLogger, "Failed to remove workdir at {}: {}", overlay->getWorkDir(), ex.what());
            }

        } else {
            quill::warning(gLogger, "Failed to destroy overlay fs at {}: {}", overlayPath, status.message);
        }

        return grpc::Status::OK;
    }

    grpc::Status DestroyOverlay(grpc::ServerContext* context, const DestroyOverlayRequest* request, DestroyOverlayResponse* response) override {
        try {
            return destroyOverlayImpl(context, request, response);
        } catch (const std::exception& ex) {
            quill::error(gLogger, "Fatal error in DestroyOverlay: {}", ex.what());
            response->set_status(EFAULT);
            response->set_detail(ex.what());
            return grpc::Status::OK;
        }
    }

public:
    FsOverlayServiceImpl(OverlayStorage *storage, OverlayManager *manager)
        : mStorage(storage)
        , mManager(manager)
    { }
};

std::unique_ptr<grpc::Server> gServer;

void handleShutdown(int signum) {
    quill::info(gLogger, "Received signal {}, shutting down pkgtoold gRPC server.", signum);
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

void setupLogging() {
    static constexpr char kTimePattern[] = "%Y-%m-%dT%H:%M:%S.%QmsZ";
    static constexpr char kMessagePattern[] = "%(time) [%(thread_id)] %(short_source_location:<12) %(log_level:<6) %(message)";
    quill::PatternFormatterOptions pattern{kMessagePattern, kTimePattern, quill::Timezone::GmtTime};

    std::shared_ptr<quill::Sink> sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("stdout");
    gLogger = quill::Frontend::create_or_get_logger("stdout", std::move(sink), pattern);

    quill::Backend::start<quill::FrontendOptions>(quill::BackendOptions{}, quill::SignalHandlerOptions{});
}

} // namespace

#define LOCALHOST_PATH "localhost:22081"
#define STORAGE_PATH "/var/lib/pkgtoold/overlays.db"

int main(int argc, const char **argv) try {
    setupLogging();

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    bool installed = isInstalled();

    if (!installed) {
        quill::info(gLogger, "pkgtoold is not installed system-wide, running over tcp.");
    } else {
        quill::info(gLogger, "Creating storage directory at " STORAGE_PATH " if it does not exist.");
        fs::create_directories("/var/lib/pkgtoold");
    }

    OverlayStorage storage{installed ? STORAGE_PATH : "overlays.db"};
    OverlayManager manager;

    for (const auto& overlay : storage.getOverlays()) {
        quill::info(gLogger, "Hydrating overlay fs at {} from storage.", overlay.getOverlayPath());
        auto err = manager.createOverlay(overlay);
        if (!err.isSuccess()) {
            quill::warning(gLogger, "Failed to hydrate overlay fs at {}: {}", overlay.getOverlayPath(), err.message);
        } else {
            quill::info(gLogger, "Overlay fs at {} hydrated successfully.", overlay.getOverlayPath());
        }
    }

    std::string address = installed ? std::format("unix://{}", pkg::pkgtooldUnixSocketPath()) : LOCALHOST_PATH;

    FsOverlayServiceImpl service{&storage, &manager};
    grpc::ServerBuilder builder;
    auto cq = builder.AddCompletionQueue();
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    //
    // The default number of threads is based on the number of cores,
    // which results in tons of wasted resources for a simple daemon like
    // this. Limit to 2 threads for now.
    // On my dev machine with an epyc 9654 i was seeing 192 threads being created, which used 32mb of rss
    // from stack space alone.
    // There are more thread pools that grpc doesn't let you configure currently, once
    // https://github.com/grpc/grpc/issues/28642 is resolved it should be possible to reduce
    // memory usage further.
    //
    grpc::ResourceQuota quota;
    quota.SetMaxThreads(2);
    builder.SetResourceQuota(quota);

    gServer = builder.BuildAndStart();
    if (gServer == nullptr) {
        quill::error(gLogger, "Failed to start pkgtoold gRPC server.");
        return 1;
    }

    quill::info(gLogger, "pkgtoold gRPC server listening on {}.", address);

    signal(SIGINT, handleShutdown);
    signal(SIGTERM, handleShutdown);

    gServer->Wait();

    quill::info(gLogger, "pkgtoold gRPC server shutting down.");

    return 0;
} catch (const std::exception& ex) {
    quill::error(gLogger, "Fatal error in pkgtoold: {}", ex.what());
    return 1;
}
