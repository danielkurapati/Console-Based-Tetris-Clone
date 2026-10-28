#pragma once
// Database: the only class that talks to the database (SQLite, one file on disk).
// It uses PREPARED STATEMENTS, so user input is never part of the SQL text
// (no SQL injection). Every call opens its own short-lived connection to the file,
// which makes the class safe to use from several server threads at once.
// SQLite itself is vendored in server/third_party/ - nothing to install.

#include <stdexcept>
#include <string>
#include <vector>

struct DbSettings {
    std::string file;   // path of the database file, e.g. "tetris.db"
};

struct NewScore {
    std::string name;
    int score = 0;
    int level = 1;
    int lines = 0;
};

struct ScoreRow {
    std::string name;
    int score = 0;
    int level = 1;
    int lines = 0;
    std::string createdAt;   // ISO text, e.g. 2026-10-13T05:59:23
};

class DbError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Database {
public:
    explicit Database(DbSettings settings);
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    void initSchema() const;                              // creates the file and the table
    bool isReachable() const;                             // can we open the file?
    unsigned long long insertScore(const NewScore& score) const;   // returns the new id
    std::vector<ScoreRow> topScores(int limit) const;     // best first

private:
    DbSettings settings_;
};
