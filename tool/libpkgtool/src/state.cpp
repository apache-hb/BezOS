#include "pkgtool/state.hpp"

#include <SQLiteCpp/SQLiteCpp.h>

namespace sqlite = SQLite;

namespace {
class IWorkspaceStateImpl final : public pkg::IWorkspaceState {
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

    static constexpr char kGetDependantPackages[] = R"sql(
        WITH RECURSIVE dependants AS (
            SELECT package, dependency FROM dependencies WHERE dependency = ? AND scope IN (?, ?, ?)
            UNION
            SELECT d.package, d.dependency FROM dependencies d
            JOIN dependants ON d.dependency = dependants.package
        )
        SELECT package FROM dependants;
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
            case pkg::DependencyScope::eDependency: return "dependency";
            case pkg::DependencyScope::eBuildDependency: return "build_dependency";
            case pkg::DependencyScope::eTestDependency: return "test_dependency";
            default: return "dependency";
        }
    }

    static pkg::DependencyScope stringToScope(const std::string& str) {
        if (str == "build_dependency") return pkg::DependencyScope::eBuildDependency;
        if (str == "test_dependency") return pkg::DependencyScope::eTestDependency;
        return pkg::DependencyScope::eDependency;
    }

public:
    IWorkspaceStateImpl(const std::filesystem::path& path)
        : mDatabase(path.string(), sqlite::OPEN_CREATE | sqlite::OPEN_READWRITE)
    {
        mDatabase.exec(kSchema);
    }

    pkg::PackageState getPackageState(const std::string& name) const override {
        sqlite::Statement stmt{mDatabase, kGetPackageState};
        stmt.bind(1, name);

        if (stmt.executeStep()) {
            auto stateStr = stmt.getColumn(0).getString();
            return stringToState(stateStr);
        }

        return pkg::PackageState::eUnknown;
    }

    void addPackage(const std::string& name) override {
        sqlite::Statement stmt{mDatabase, kAddPackage};
        stmt.bind(1, name);
        stmt.exec();
    }

    void setPackageState(const std::string& name, pkg::PackageState state, bool recursive) override {
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
        sqlite::Statement stmt{mDatabase, kAddDependency};
        stmt.bind(1, package);
        stmt.bind(2, dependency);
        stmt.bind(3, scopeToString(scope));
        stmt.exec();
    }

    std::vector<std::string> getDependantPackages(const std::string& name, pkg::DependencyScope scopes) const override {
        std::vector<std::string> result;

        sqlite::Statement query{mDatabase, kGetDependantPackages};
        query.bind(1, name);

        // A bit of a dumb hack, but theres no way to bind an array of values in sqlite
        query.bind(2, ((int)scopes & (int)pkg::DependencyScope::eDependency) != 0 ? "dependency" : "none");
        query.bind(3, ((int)scopes & (int)pkg::DependencyScope::eBuildDependency) != 0 ? "build_dependency" : "none");
        query.bind(4, ((int)scopes & (int)pkg::DependencyScope::eTestDependency) != 0 ? "test_dependency" : "none");

        while (query.executeStep()) {
            result.push_back(query.getColumn(0).getString());
        }

        result.push_back(name);

        return result;
    }
};
}

std::shared_ptr<pkg::IWorkspaceState> pkg::IWorkspaceState::ofSqlite(const std::filesystem::path& path) {
    return std::make_shared<IWorkspaceStateImpl>(path);
}
