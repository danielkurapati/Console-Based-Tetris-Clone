#pragma once
// ApiServer: the REST API of the leaderboard.
//
//   GET  /health            liveness + database check
//   GET  /scores?limit=10   top scores, best first          (public)
//   POST /scores            store a score, needs X-API-Key  (JSON body)
//
// It checks the API key, validates the JSON, and asks Database to store or read scores.

#include <memory>

#include "Database.h"
#include "Settings.h"

namespace httplib {
class Server;   // full definition only in ApiServer.cpp (keeps this header light)
}

class ApiServer {
public:
    ApiServer(Settings settings, Database& database);
    ~ApiServer();
    ApiServer(const ApiServer&) = delete;
    ApiServer& operator=(const ApiServer&) = delete;

    // Starts listening on 127.0.0.1 and blocks. Returns false if the port could not be opened.
    bool run();

private:
    void registerRoutes();

    Settings settings_;
    Database& db_;
    std::unique_ptr<httplib::Server> server_;
};
