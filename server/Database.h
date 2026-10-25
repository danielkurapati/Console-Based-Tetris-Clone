#pragma once

#include <stdexcept>
#include <string>
#include <vector>

struct DbSettings {
    std::string host;
    unsigned int port = 3306;
    std::string user;
    std::string password;
    std::string name;
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
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    void initSchema() const;                              // CREATE TABLE IF NOT EXISTS
    bool isReachable() const;                             // can we log in?
    unsigned long long insertScore(const NewScore& score) const;   // returns the new id
    std::vector<ScoreRow> topScores(int limit) const;     // best first

private:
    DbSettings settings_;
};
