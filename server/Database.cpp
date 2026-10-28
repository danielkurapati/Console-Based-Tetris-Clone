#include "Database.h"

#include <utility>

#include "third_party/sqlite3.h"

namespace {

// RAII wrapper: the connection is closed when the object goes out of scope.
class Connection {
public:
    explicit Connection(const DbSettings& s) {
        const int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
        if (sqlite3_open_v2(s.file.c_str(), &db_, flags, nullptr) != SQLITE_OK) {
            std::string message = db_ != nullptr ? sqlite3_errmsg(db_) : "out of memory";
            sqlite3_close(db_);
            throw DbError("cannot open '" + s.file + "': " + message);
        }
        // Several requests may touch the file at once: wait up to 3 s instead of failing.
        sqlite3_busy_timeout(db_, 3000);
    }
    ~Connection() { sqlite3_close(db_); }
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    sqlite3* get() const { return db_; }

    void run(const char* sql) const {
        char* error = nullptr;
        if (sqlite3_exec(db_, sql, nullptr, nullptr, &error) != SQLITE_OK) {
            std::string message = error != nullptr ? error : sqlite3_errmsg(db_);
            sqlite3_free(error);
            throw DbError(message);
        }
    }

private:
    sqlite3* db_ = nullptr;
};

// RAII wrapper for a prepared statement.
class Statement {
public:
    Statement(sqlite3* db, const char* sql) : db_(db) {
        if (sqlite3_prepare_v2(db, sql, -1, &stmt_, nullptr) != SQLITE_OK)
            throw DbError(std::string("prepare: ") + sqlite3_errmsg(db));
    }
    ~Statement() { sqlite3_finalize(stmt_); }
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    sqlite3_stmt* get() const { return stmt_; }

    [[noreturn]] void fail(const char* step) const {
        throw DbError(std::string(step) + ": " + sqlite3_errmsg(db_));
    }

private:
    sqlite3* db_;
    sqlite3_stmt* stmt_ = nullptr;
};

std::string textColumn(sqlite3_stmt* stmt, int column) {
    const unsigned char* text = sqlite3_column_text(stmt, column);
    const int bytes = sqlite3_column_bytes(stmt, column);
    return text == nullptr ? std::string() : std::string(reinterpret_cast<const char*>(text), bytes);
}

}  // namespace

Database::Database(DbSettings settings) : settings_(std::move(settings)) {}

void Database::initSchema() const {
    Connection conn(settings_);
    // WAL lets readers and the writer work at the same time. The setting is stored in the file.
    conn.run("PRAGMA journal_mode=WAL;");
    conn.run(
        "CREATE TABLE IF NOT EXISTS scores ("
        " id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        " name          TEXT    NOT NULL,"
        " score         INTEGER NOT NULL,"
        " level         INTEGER NOT NULL,"
        " lines_cleared INTEGER NOT NULL,"
        " created_at    TEXT    NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%S','now','localtime')));"
        "CREATE INDEX IF NOT EXISTS idx_scores_score ON scores (score DESC);");
}

bool Database::isReachable() const {
    try {
        Connection conn(settings_);
        conn.run("SELECT 1;");
        return true;
    } catch (const DbError&) {
        return false;
    }
}

unsigned long long Database::insertScore(const NewScore& s) const {
    Connection conn(settings_);
    Statement stmt(conn.get(),
                   "INSERT INTO scores (name, score, level, lines_cleared) VALUES (?, ?, ?, ?)");

    if (sqlite3_bind_text(stmt.get(), 1, s.name.data(), static_cast<int>(s.name.size()),
                          SQLITE_TRANSIENT) != SQLITE_OK) stmt.fail("bind name");
    if (sqlite3_bind_int(stmt.get(), 2, s.score) != SQLITE_OK) stmt.fail("bind score");
    if (sqlite3_bind_int(stmt.get(), 3, s.level) != SQLITE_OK) stmt.fail("bind level");
    if (sqlite3_bind_int(stmt.get(), 4, s.lines) != SQLITE_OK) stmt.fail("bind lines");

    if (sqlite3_step(stmt.get()) != SQLITE_DONE) stmt.fail("execute insert");
    return static_cast<unsigned long long>(sqlite3_last_insert_rowid(conn.get()));
}

std::vector<ScoreRow> Database::topScores(int limit) const {
    Connection conn(settings_);
    Statement stmt(conn.get(),
                   "SELECT name, score, level, lines_cleared, created_at "
                   "FROM scores ORDER BY score DESC, created_at ASC, id ASC LIMIT ?");

    if (sqlite3_bind_int(stmt.get(), 1, limit) != SQLITE_OK) stmt.fail("bind limit");

    std::vector<ScoreRow> rows;
    for (;;) {
        const int rc = sqlite3_step(stmt.get());
        if (rc == SQLITE_DONE) break;
        if (rc != SQLITE_ROW) stmt.fail("fetch row");

        ScoreRow row;
        row.name = textColumn(stmt.get(), 0);
        row.score = sqlite3_column_int(stmt.get(), 1);
        row.level = sqlite3_column_int(stmt.get(), 2);
        row.lines = sqlite3_column_int(stmt.get(), 3);
        row.createdAt = textColumn(stmt.get(), 4);
        rows.push_back(std::move(row));
    }
    return rows;
}
