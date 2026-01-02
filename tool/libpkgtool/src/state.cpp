#include "pkgtool/state.hpp"

#include <SQLiteCpp/SQLiteCpp.h>

#include <quill/Frontend.h>
#include <quill/LogMacros.h>

namespace sqlite = SQLite;

namespace {
class WorkspaceStateImpl final : public pkg::IWorkspaceState {
    static inline auto logger() {
        static auto it = quill::Frontend::create_or_get_logger("WorkspaceStateImpl", quill::Frontend::get_logger("root"));
        return it;
    }

    sqlite::Database mDatabase;

    static constexpr char kSchema[] = R"sql(
        CREATE TABLE IF NOT EXISTS targets (
            name TEXT PRIMARY KEY,
            state TEXT NOT NULL
        );

        DELETE FROM targets WHERE name = '';

        DROP TABLE IF EXISTS dependencies;

        CREATE TABLE dependencies (
            package TEXT NOT NULL,
            dependency TEXT NOT NULL,
            scope TEXT NOT NULL DEFAULT 'dependency',
            UNIQUE(package, dependency, scope)
        );
    )sql";

    static constexpr char kAddPackage[] = R"sql(
        INSERT OR IGNORE INTO targets (name, state) VALUES (?, 'unknown');
    )sql";

    static constexpr char kGetPackageState[] = R"sql(
        SELECT state FROM targets WHERE name = ?;
    )sql";

    static constexpr char kSetPackageStateSingle[] = R"sql(
        UPDATE targets SET state = ? WHERE name = ?;
    )sql";

    static constexpr char kSetPackageStateRecursive[] = R"sql(
        WITH RECURSIVE deps(name) AS (
            SELECT ? UNION
            SELECT dependency FROM dependencies JOIN deps ON dependencies.package = deps.name
        )
        UPDATE targets SET state = ? WHERE name IN (SELECT name FROM deps);
    )sql";

    static constexpr char kAddDependency[] = R"sql(
        INSERT OR IGNORE INTO dependencies (package, dependency, scope) VALUES (?, ?, ?);
    )sql";

    static constexpr char kGetReverseDependencies[] = R"sql(
        WITH RECURSIVE dependants AS (
            SELECT package, dependency FROM dependencies WHERE dependency = ? AND scope IN (SELECT name FROM scopes)
            UNION
            SELECT d.package, d.dependency FROM dependencies d
            JOIN dependants ON d.dependency = dependants.package
        )
        SELECT DISTINCT package FROM dependants;
    )sql";

    static constexpr char kGetAllDependencies[] = R"sql(
        WITH RECURSIVE deps AS (
            SELECT dependency FROM dependencies WHERE package = ? AND scope IN (SELECT name FROM scopes)
            UNION
            SELECT d.dependency FROM dependencies d
            JOIN deps ON d.package = deps.dependency
        )
        SELECT DISTINCT dependency FROM deps;
    )sql";

    static constexpr char kGetDirectDependencies[] = R"sql(
        SELECT DISTINCT dependency FROM dependencies WHERE package = ? AND scope IN (SELECT name FROM scopes);
    )sql";

    static constexpr char kCreateScopeTable[] = R"sql(
        CREATE TEMPORARY TABLE IF NOT EXISTS scopes (
            name TEXT PRIMARY KEY
        );
    )sql";

    static constexpr char kClearScopes[] = R"sql(
        DELETE FROM scopes;
    )sql";

    static constexpr char kAddScope[] = R"sql(
        INSERT INTO scopes (name) VALUES (?);
    )sql";

    static std::string stateToString(pkg::PackageState state) {
        switch (state) {
            case pkg::PackageState::eUnknown: return "unknown";
            case pkg::PackageState::eFetched: return "fetched";
            case pkg::PackageState::eConfigured: return "configured";
            case pkg::PackageState::eBuilt: return "built";
            case pkg::PackageState::eInstalled: return "installed";
            default: return "unknown";
        }
    }

    static pkg::PackageState stringToState(const std::string& str) {
        if (str == "fetched") return pkg::PackageState::eFetched;
        if (str == "configured") return pkg::PackageState::eConfigured;
        if (str == "built") return pkg::PackageState::eBuilt;
        if (str == "installed") return pkg::PackageState::eInstalled;
        return pkg::PackageState::eUnknown;
    }

    static std::string scopeToString(pkg::DependencyScope scope) {
        switch (scope) {
            case pkg::DependencyScope::ePublicDependency: return "public_dependency";
            case pkg::DependencyScope::ePrivateDependency: return "private_dependency";
            default: return "dependency";
        }
    }

    static pkg::DependencyScope stringToScope(const std::string& str) {
        if (str == "public_dependency") return pkg::DependencyScope::ePublicDependency;
        if (str == "private_dependency") return pkg::DependencyScope::ePrivateDependency;
        return pkg::DependencyScope::ePublicDependency; // default
    }

    void addScopesToQuery(pkg::DependencyScope scopes) const {
        sqlite::Statement clear{mDatabase, kClearScopes};
        clear.exec();

        if (pkg::testBit(scopes, pkg::DependencyScope::ePublicDependency)) {
            sqlite::Statement insert{mDatabase, kAddScope};
            insert.bind(1, "public_dependency");
            insert.exec();
        }

        if (pkg::testBit(scopes, pkg::DependencyScope::ePrivateDependency)) {
            sqlite::Statement insert{mDatabase, kAddScope};
            insert.bind(1, "private_dependency");
            insert.exec();
        }
    }

public:
    WorkspaceStateImpl(const std::filesystem::path& path)
        : mDatabase(path.string(), sqlite::OPEN_CREATE | sqlite::OPEN_READWRITE)
    {
        mDatabase.exec(kSchema);
        mDatabase.exec(kCreateScopeTable);
    }

    pkg::PackageState getPackageState(const std::string& name) const override {
        sqlite::Statement stmt{mDatabase, kGetPackageState};
        stmt.bind(1, name);

        if (stmt.executeStep()) {
            auto stateStr = stmt.getColumn(0).getString();
            auto state = stringToState(stateStr);
            LOG_TRACE_L2(logger(), "Package '{}' has state '{}'", name, stateStr);
            return state;
        }

        LOG_TRACE_L2(logger(), "Package '{}' has unknown state", name);
        return pkg::PackageState::eUnknown;
    }

    void addPackage(const std::string& name) override {
        LOG_TRACE_L2(logger(), "Adding package '{}' to workspace state", name);

        sqlite::Statement stmt{mDatabase, kAddPackage};
        stmt.bind(1, name);
        stmt.exec();
    }

    void lowerPackageState(const std::string& name, pkg::PackageState state) override {
        LOG_TRACE_L1(logger(), "Lowering package '{}' state to '{}'", name, stateToString(state));

        sqlite::Statement stmt{mDatabase, kGetPackageState};
        stmt.bind(1, name);

        if (stmt.executeStep()) {
            auto currentStateStr = stmt.getColumn(0).getString();
            auto currentState = stringToState(currentStateStr);

            if (currentState > state) {
                sqlite::Statement updateStmt{mDatabase, kSetPackageStateSingle};
                updateStmt.bind(1, stateToString(state));
                updateStmt.bind(2, name);
                updateStmt.exec();
            }
        }
    }

    void setPackageState(const std::string& name, pkg::PackageState state, bool recursive) override {
        LOG_TRACE_L1(logger(), "Setting package '{}' state to '{}' (recursive={})", name, stateToString(state), recursive);

        if (!recursive) {
            sqlite::Statement stmt{mDatabase, kSetPackageStateSingle};
            stmt.bind(1, stateToString(state));
            stmt.bind(2, name);
            stmt.exec();
        } else {
            sqlite::Statement stmt{mDatabase, kSetPackageStateRecursive};
            stmt.bind(1, name);
            stmt.bind(2, stateToString(state));
            stmt.exec();
        }
    }

    void addDependency(const std::string& package, const std::string& dependency, pkg::DependencyScope scope) override {
        LOG_TRACE_L1(logger(), "Adding dependency '{}' to package '{}' with scope '{}'", dependency, package, scopeToString(scope));

        sqlite::Statement stmt{mDatabase, kAddDependency};
        stmt.bind(1, package);
        stmt.bind(2, dependency);
        stmt.bind(3, scopeToString(scope));
        stmt.exec();
    }

    std::vector<std::string> getReverseDependencies(const std::string& name, pkg::DependencyScope scopes) const override {
        std::vector<std::string> result;

        addScopesToQuery(scopes);

        sqlite::Statement query{mDatabase, kGetReverseDependencies};
        query.bind(1, name);

        while (query.executeStep()) {
            result.push_back(query.getColumn(0).getString());
        }

        return result;
    }

    std::vector<std::string> getAllDependencies(const std::string& name, pkg::DependencyScope scopes) const override {
        std::vector<std::string> result;

        addScopesToQuery(scopes);

        sqlite::Statement query{mDatabase, kGetAllDependencies};
        query.bind(1, name);

        while (query.executeStep()) {
            result.push_back(query.getColumn(0).getString());
        }

        return result;
    }

    std::vector<std::string> getDirectDependencies(const std::string& name, pkg::DependencyScope scopes) const override {
        std::vector<std::string> result;

        addScopesToQuery(scopes);

        sqlite::Statement query{mDatabase, kGetDirectDependencies};
        query.bind(1, name);

        while (query.executeStep()) {
            result.push_back(query.getColumn(0).getString());
        }

        return result;
    }
};
}

std::shared_ptr<pkg::IWorkspaceState> pkg::IWorkspaceState::ofSqlite(const std::filesystem::path& path) {
    return std::make_shared<WorkspaceStateImpl>(path);
}
