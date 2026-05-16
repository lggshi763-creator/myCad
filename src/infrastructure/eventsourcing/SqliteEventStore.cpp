#include <mycad/domain/EventTypeRegistry.hpp>
#include <mycad/infrastructure/SqliteEventStore.hpp>
#include <sqlite3.h>

#include <array>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace mycad::infrastructure {

// ---------------------------------------------------------------------------
// Helpers — AggregateId ↔ BLOB
// ---------------------------------------------------------------------------

namespace {

/// @brief Binds an AggregateId as a 16-byte BLOB to a prepared statement.
void bindId(sqlite3_stmt* stmt, int col, const domain::AggregateId& id) {
    sqlite3_bind_blob(
        stmt, col, id.bytes.data(), static_cast<int>(id.bytes.size()), SQLITE_TRANSIENT);
}

/// @brief Reads a 16-byte BLOB column back into an AggregateId.
domain::AggregateId columnId(sqlite3_stmt* stmt, int col) {
    domain::AggregateId id{};
    const void* data = sqlite3_column_blob(stmt, col);
    if (data && sqlite3_column_bytes(stmt, col) == static_cast<int>(id.bytes.size())) {
        std::memcpy(id.bytes.data(), data, id.bytes.size());
    }
    return id;
}

/// @brief RAII helper that finalises a prepared statement.
struct StmtGuard {
    sqlite3_stmt* s{nullptr};
    ~StmtGuard() {
        if (s)
            sqlite3_finalize(s);
    }
};

}  // namespace

// ---------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------

struct SqliteEventStore::Impl {
    sqlite3* db{nullptr};
    std::string path;
    mutable std::mutex mu;
    std::unordered_map<std::string, SerializerFn> serializers;
};

// ---------------------------------------------------------------------------
// Schema DDL
// ---------------------------------------------------------------------------

namespace {

constexpr const char* kDDL = R"sql(
CREATE TABLE IF NOT EXISTS events (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    aggregate_id BLOB    NOT NULL,
    version      INTEGER NOT NULL,
    type_name    TEXT    NOT NULL,
    payload      TEXT    NOT NULL DEFAULT '{}',
    occurred_at  INTEGER NOT NULL
);
CREATE UNIQUE INDEX IF NOT EXISTS idx_events_agg_ver
    ON events (aggregate_id, version);
)sql";

}  // namespace

// ---------------------------------------------------------------------------
// Constructor / destructor
// ---------------------------------------------------------------------------

SqliteEventStore::SqliteEventStore(std::string_view dbPath) : impl_(std::make_unique<Impl>()) {
    impl_->path = std::string(dbPath);

    if (sqlite3_open(impl_->path.c_str(), &impl_->db) != SQLITE_OK) {
        const std::string msg = impl_->db ? sqlite3_errmsg(impl_->db) : "unknown error";
        sqlite3_close(impl_->db);
        impl_->db = nullptr;
        throw std::runtime_error("SqliteEventStore: cannot open '" + impl_->path + "': " + msg);
    }

    // Enable WAL mode for better concurrent read performance.
    sqlite3_exec(impl_->db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(impl_->db, "PRAGMA foreign_keys=ON;", nullptr, nullptr, nullptr);

    char* errmsg = nullptr;
    if (sqlite3_exec(impl_->db, kDDL, nullptr, nullptr, &errmsg) != SQLITE_OK) {
        const std::string msg = errmsg ? errmsg : "DDL failed";
        sqlite3_free(errmsg);
        sqlite3_close(impl_->db);
        impl_->db = nullptr;
        throw std::runtime_error("SqliteEventStore: schema init failed: " + msg);
    }
}

SqliteEventStore::~SqliteEventStore() {
    if (impl_ && impl_->db) {
        sqlite3_close(impl_->db);
        impl_->db = nullptr;
    }
}

// ---------------------------------------------------------------------------
// registerSerializer
// ---------------------------------------------------------------------------

void SqliteEventStore::registerSerializer(std::string typeName, SerializerFn fn) {
    std::lock_guard lock(impl_->mu);
    impl_->serializers[std::move(typeName)] = std::move(fn);
}

// ---------------------------------------------------------------------------
// dbPath
// ---------------------------------------------------------------------------

std::string_view SqliteEventStore::dbPath() const noexcept {
    return impl_->path;
}

// ---------------------------------------------------------------------------
// latestVersion
// ---------------------------------------------------------------------------

domain::Version SqliteEventStore::latestVersion(domain::AggregateId aggregateId) {
    std::lock_guard lock(impl_->mu);

    constexpr const char* kSQL = "SELECT MAX(version) FROM events WHERE aggregate_id = ?;";

    StmtGuard g;
    if (sqlite3_prepare_v2(impl_->db, kSQL, -1, &g.s, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("SqliteEventStore::latestVersion prepare: ") +
                                 sqlite3_errmsg(impl_->db));
    }
    bindId(g.s, 1, aggregateId);

    if (sqlite3_step(g.s) == SQLITE_ROW && sqlite3_column_type(g.s, 0) != SQLITE_NULL) {
        return domain::Version{static_cast<std::uint32_t>(sqlite3_column_int64(g.s, 0))};
    }
    return domain::Version{0};
}

// ---------------------------------------------------------------------------
// append
// ---------------------------------------------------------------------------

void SqliteEventStore::append(domain::AggregateId aggregateId,
                              domain::Version expectedVersion,
                              std::span<const domain::DomainEvent* const> events) {
    if (events.empty()) {
        return;
    }

    std::lock_guard lock(impl_->mu);

    // --- Optimistic concurrency check (within the same lock) ---
    {
        constexpr const char* kVerSQL = "SELECT MAX(version) FROM events WHERE aggregate_id = ?;";
        StmtGuard g;
        sqlite3_prepare_v2(impl_->db, kVerSQL, -1, &g.s, nullptr);
        bindId(g.s, 1, aggregateId);

        domain::Version stored{0};
        if (sqlite3_step(g.s) == SQLITE_ROW && sqlite3_column_type(g.s, 0) != SQLITE_NULL) {
            stored = domain::Version{static_cast<std::uint32_t>(sqlite3_column_int64(g.s, 0))};
        }

        if (stored != expectedVersion) {
            throw domain::ConcurrencyError(aggregateId, expectedVersion, stored);
        }
    }

    // --- Insert all events in a single transaction ---
    sqlite3_exec(impl_->db, "BEGIN;", nullptr, nullptr, nullptr);

    constexpr const char* kInsSQL =
        "INSERT INTO events (aggregate_id, version, type_name, payload, occurred_at) "
        "VALUES (?, ?, ?, ?, ?);";

    StmtGuard ins;
    if (sqlite3_prepare_v2(impl_->db, kInsSQL, -1, &ins.s, nullptr) != SQLITE_OK) {
        sqlite3_exec(impl_->db, "ROLLBACK;", nullptr, nullptr, nullptr);
        throw std::runtime_error(std::string("SqliteEventStore::append prepare: ") +
                                 sqlite3_errmsg(impl_->db));
    }

    for (const domain::DomainEvent* ev : events) {
        // Serialise payload (JSON) if a serialiser is registered.
        std::string payload = "{}";
        const auto it = impl_->serializers.find(ev->typeName());
        if (it != impl_->serializers.end()) {
            payload = it->second(*ev);
        }

        sqlite3_reset(ins.s);
        bindId(ins.s, 1, aggregateId);
        sqlite3_bind_int64(ins.s, 2, static_cast<sqlite3_int64>(ev->aggregateVersion().value));
        sqlite3_bind_text(ins.s, 3, ev->typeName().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(ins.s, 4, payload.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(ins.s, 5, static_cast<sqlite3_int64>(ev->occurredAtMs()));

        if (sqlite3_step(ins.s) != SQLITE_DONE) {
            sqlite3_exec(impl_->db, "ROLLBACK;", nullptr, nullptr, nullptr);
            throw std::runtime_error(std::string("SqliteEventStore::append insert: ") +
                                     sqlite3_errmsg(impl_->db));
        }
    }

    sqlite3_exec(impl_->db, "COMMIT;", nullptr, nullptr, nullptr);
}

// ---------------------------------------------------------------------------
// Internal helper: reconstruct DomainEvent objects from rows
// ---------------------------------------------------------------------------

namespace {

std::vector<std::unique_ptr<domain::DomainEvent>> reconstructFromRows(sqlite3_stmt* stmt) {
    domain::EventTypeRegistry& reg = domain::EventTypeRegistry::instance();
    std::vector<std::unique_ptr<domain::DomainEvent>> result;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const domain::AggregateId aggregateId = columnId(stmt, 0);
        const auto version =
            domain::Version{static_cast<std::uint32_t>(sqlite3_column_int64(stmt, 1))};
        const std::string typeName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const auto occurredAtMs = static_cast<std::uint64_t>(sqlite3_column_int64(stmt, 3));

        const domain::EventTypeRegistry::FactoryFn* factory = reg.findFactory(typeName);
        if (!factory) {
            throw std::runtime_error("SqliteEventStore::load: no factory for type '" + typeName +
                                     "'");
        }
        result.push_back((*factory)(aggregateId, version, occurredAtMs));
    }
    return result;
}

}  // namespace

// ---------------------------------------------------------------------------
// load
// ---------------------------------------------------------------------------

std::vector<std::unique_ptr<domain::DomainEvent>>
SqliteEventStore::load(domain::AggregateId aggregateId) {
    std::lock_guard lock(impl_->mu);

    constexpr const char* kSQL = "SELECT aggregate_id, version, type_name, occurred_at "
                                 "FROM events WHERE aggregate_id = ? ORDER BY version ASC;";

    StmtGuard g;
    if (sqlite3_prepare_v2(impl_->db, kSQL, -1, &g.s, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("SqliteEventStore::load prepare: ") +
                                 sqlite3_errmsg(impl_->db));
    }
    bindId(g.s, 1, aggregateId);
    return reconstructFromRows(g.s);
}

// ---------------------------------------------------------------------------
// loadSince
// ---------------------------------------------------------------------------

std::vector<std::unique_ptr<domain::DomainEvent>>
SqliteEventStore::loadSince(domain::AggregateId aggregateId, domain::Version fromVersion) {
    std::lock_guard lock(impl_->mu);

    constexpr const char* kSQL =
        "SELECT aggregate_id, version, type_name, occurred_at "
        "FROM events WHERE aggregate_id = ? AND version >= ? ORDER BY version ASC;";

    StmtGuard g;
    if (sqlite3_prepare_v2(impl_->db, kSQL, -1, &g.s, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("SqliteEventStore::loadSince prepare: ") +
                                 sqlite3_errmsg(impl_->db));
    }
    bindId(g.s, 1, aggregateId);
    sqlite3_bind_int64(g.s, 2, static_cast<sqlite3_int64>(fromVersion.value));
    return reconstructFromRows(g.s);
}

// ---------------------------------------------------------------------------
// loadRows  (payload-aware, used by T5 File → Open replay)
// ---------------------------------------------------------------------------

std::vector<SqliteEventStore::RawRow>
SqliteEventStore::loadRows(domain::AggregateId aggregateId) const {
    std::lock_guard lock(impl_->mu);

    constexpr const char* kSQL = "SELECT aggregate_id, version, type_name, payload, occurred_at "
                                 "FROM events WHERE aggregate_id = ? ORDER BY version ASC;";

    StmtGuard g;
    if (sqlite3_prepare_v2(impl_->db, kSQL, -1, &g.s, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("SqliteEventStore::loadRows prepare: ") +
                                 sqlite3_errmsg(impl_->db));
    }
    bindId(g.s, 1, aggregateId);

    std::vector<RawRow> rows;
    while (sqlite3_step(g.s) == SQLITE_ROW) {
        RawRow row;
        row.aggregateId = columnId(g.s, 0);
        row.version = domain::Version{static_cast<std::uint32_t>(sqlite3_column_int64(g.s, 1))};
        row.typeName = reinterpret_cast<const char*>(sqlite3_column_text(g.s, 2));
        row.payload = reinterpret_cast<const char*>(sqlite3_column_text(g.s, 3));
        row.occurredAtMs = static_cast<std::uint64_t>(sqlite3_column_int64(g.s, 4));
        rows.push_back(std::move(row));
    }
    return rows;
}

// ---------------------------------------------------------------------------
// allAggregateIds  (used by File → Open to enumerate all aggregates)
// ---------------------------------------------------------------------------

std::vector<domain::AggregateId> SqliteEventStore::allAggregateIds() const {
    std::lock_guard lock(impl_->mu);

    constexpr const char* kSQL = "SELECT DISTINCT aggregate_id FROM events;";

    StmtGuard g;
    if (sqlite3_prepare_v2(impl_->db, kSQL, -1, &g.s, nullptr) != SQLITE_OK) {
        throw std::runtime_error(std::string("SqliteEventStore::allAggregateIds prepare: ") +
                                 sqlite3_errmsg(impl_->db));
    }

    std::vector<domain::AggregateId> ids;
    while (sqlite3_step(g.s) == SQLITE_ROW) {
        ids.push_back(columnId(g.s, 0));
    }
    return ids;
}

}  // namespace mycad::infrastructure
