#include "Database.h"

#include <algorithm>
#include <cstring>
#include <type_traits>
#include <utility>

// The header is called mysql/mysql.h for MySQL and mariadb/mysql.h for MariaDB.
#if __has_include(<mysql/mysql.h>)
    #include <mysql/mysql.h>
#elif __has_include(<mariadb/mysql.h>)
    #include <mariadb/mysql.h>
#else
    #include <mysql.h>
#endif

#ifndef MYSQL_DATA_TRUNCATED
    #define MYSQL_DATA_TRUNCATED 101
#endif

namespace {

// MySQL 8 uses 'bool' for these flags, MariaDB uses 'my_bool'. Take whatever the library uses.
using BindBool = std::remove_pointer<decltype(MYSQL_BIND::is_null)>::type;

// RAII wrapper: the connection is closed when the object goes out of scope.
class Connection {
public:
    explicit Connection(const DbSettings& s) {
        conn_ = mysql_init(nullptr);
        if (conn_ == nullptr) throw DbError("mysql_init failed (out of memory)");
        unsigned int timeoutSeconds = 3;
        mysql_options(conn_, MYSQL_OPT_CONNECT_TIMEOUT, &timeoutSeconds);
        mysql_options(conn_, MYSQL_SET_CHARSET_NAME, "utf8mb4");
        if (mysql_real_connect(conn_, s.host.c_str(), s.user.c_str(), s.password.c_str(),
                               s.name.c_str(), s.port, nullptr, 0) == nullptr) {
            std::string message = mysql_error(conn_);
            mysql_close(conn_);
            throw DbError(message);
        }
    }
    ~Connection() { mysql_close(conn_); }
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    MYSQL* get() const { return conn_; }

private:
    MYSQL* conn_ = nullptr;
};

// RAII wrapper for a prepared statement.
class Statement {
public:
    Statement(MYSQL* conn, const std::string& sql) {
        stmt_ = mysql_stmt_init(conn);
        if (stmt_ == nullptr) throw DbError("mysql_stmt_init failed (out of memory)");
        if (mysql_stmt_prepare(stmt_, sql.c_str(), static_cast<unsigned long>(sql.size())) != 0) {
            std::string message = mysql_stmt_error(stmt_);
            mysql_stmt_close(stmt_);
            throw DbError(message);
        }
    }
    ~Statement() { mysql_stmt_close(stmt_); }
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    MYSQL_STMT* get() const { return stmt_; }

    [[noreturn]] void fail(const char* step) const {
        throw DbError(std::string(step) + ": " + mysql_stmt_error(stmt_));
    }

private:
    MYSQL_STMT* stmt_ = nullptr;
};

}

Database::Database(DbSettings settings) : settings_(std::move(settings)) {
    if (mysql_library_init(0, nullptr, nullptr) != 0)
        throw DbError("could not initialise the MySQL client library");
}

Database::~Database() { mysql_library_end(); }

void Database::initSchema() const {
    Connection conn(settings_);
    const std::string sql =
        "CREATE TABLE IF NOT EXISTS scores ("
        " id            INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
        " name          VARCHAR(20)  NOT NULL,"
        " score         INT UNSIGNED NOT NULL,"
        " level         SMALLINT UNSIGNED NOT NULL,"
        " lines_cleared SMALLINT UNSIGNED NOT NULL,"
        " created_at    TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        " INDEX idx_scores_score (score))";
    if (mysql_real_query(conn.get(), sql.c_str(), static_cast<unsigned long>(sql.size())) != 0)
        throw DbError(mysql_error(conn.get()));
}

bool Database::isReachable() const {
    try {
        Connection conn(settings_);
        return true;
    } catch (const DbError&) {
        return false;
    }
}

unsigned long long Database::insertScore(const NewScore& s) const {
    Connection conn(settings_);
    Statement stmt(conn.get(),
                   "INSERT INTO scores (name, score, level, lines_cleared) VALUES (?, ?, ?, ?)");

    int score = s.score;
    int level = s.level;
    int lines = s.lines;
    unsigned long nameLength = static_cast<unsigned long>(s.name.size());

    MYSQL_BIND params[4];
    std::memset(params, 0, sizeof(params));
    params[0].buffer_type = MYSQL_TYPE_STRING;
    params[0].buffer = const_cast<char*>(s.name.data());
    params[0].buffer_length = nameLength;
    params[0].length = &nameLength;
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer = &score;
    params[2].buffer_type = MYSQL_TYPE_LONG;
    params[2].buffer = &level;
    params[3].buffer_type = MYSQL_TYPE_LONG;
    params[3].buffer = &lines;

    if (mysql_stmt_bind_param(stmt.get(), params) != 0) stmt.fail("bind parameters");
    if (mysql_stmt_execute(stmt.get()) != 0) stmt.fail("execute insert");
    return mysql_stmt_insert_id(stmt.get());
}

std::vector<ScoreRow> Database::topScores(int limit) const {
    Connection conn(settings_);
    Statement stmt(conn.get(),
                   "SELECT name, score, level, lines_cleared,"
                   " DATE_FORMAT(created_at, '%Y-%m-%dT%H:%i:%s') "
                   "FROM scores ORDER BY score DESC, created_at ASC, id ASC LIMIT ?");

    int limitValue = limit;
    MYSQL_BIND param;
    std::memset(&param, 0, sizeof(param));
    param.buffer_type = MYSQL_TYPE_LONG;
    param.buffer = &limitValue;
    if (mysql_stmt_bind_param(stmt.get(), &param) != 0) stmt.fail("bind limit");
    if (mysql_stmt_execute(stmt.get()) != 0) stmt.fail("execute select");

    // Buffers that receive one row at a time.
    char nameBuf[128];
    char dateBuf[32];
    unsigned long nameLength = 0, dateLength = 0;
    int score = 0, level = 0, lines = 0;
    BindBool isNull[5] = {};
    BindBool hasError[5] = {};

    MYSQL_BIND out[5];
    std::memset(out, 0, sizeof(out));
    out[0].buffer_type = MYSQL_TYPE_STRING;
    out[0].buffer = nameBuf;
    out[0].buffer_length = sizeof(nameBuf);
    out[0].length = &nameLength;
    out[1].buffer_type = MYSQL_TYPE_LONG;
    out[1].buffer = &score;
    out[2].buffer_type = MYSQL_TYPE_LONG;
    out[2].buffer = &level;
    out[3].buffer_type = MYSQL_TYPE_LONG;
    out[3].buffer = &lines;
    out[4].buffer_type = MYSQL_TYPE_STRING;
    out[4].buffer = dateBuf;
    out[4].buffer_length = sizeof(dateBuf);
    out[4].length = &dateLength;
    for (int i = 0; i < 5; ++i) {
        out[i].is_null = &isNull[i];
        out[i].error = &hasError[i];
    }

    if (mysql_stmt_bind_result(stmt.get(), out) != 0) stmt.fail("bind result");
    if (mysql_stmt_store_result(stmt.get()) != 0) stmt.fail("store result");

    std::vector<ScoreRow> rows;
    for (;;) {
        int rc = mysql_stmt_fetch(stmt.get());
        if (rc == MYSQL_NO_DATA) break;
        if (rc != 0 && rc != MYSQL_DATA_TRUNCATED) stmt.fail("fetch row");

        ScoreRow row;
        row.name.assign(nameBuf, std::min<unsigned long>(nameLength, sizeof(nameBuf)));
        row.score = score;
        row.level = level;
        row.lines = lines;
        row.createdAt.assign(dateBuf, std::min<unsigned long>(dateLength, sizeof(dateBuf)));
        rows.push_back(std::move(row));
    }
    return rows;
}
