#include "ApiServer.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <string>

#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
#endif

#include "../src/third_party/httplib.h"
#include "../src/third_party/json.hpp"

using nlohmann::json;

namespace {

constexpr std::size_t kNameMax = 20;
constexpr long long kScoreMax = 10000000;
constexpr int kDefaultLimit = 10;
constexpr int kMaxLimit = 50;
constexpr std::size_t kMaxBodyBytes = 4096;   // a score is tiny; refuse big bodies

void sendJson(httplib::Response& res, int status, const json& body) {
    res.status = status;
    res.set_content(body.dump(), "application/json");
}

void sendError(httplib::Response& res, int status, const std::string& message) {
    sendJson(res, status, json{{"error", message}});
}

// Compares two strings without stopping at the first difference, so the time taken
// does not reveal how much of a guessed API key was correct.
bool constantTimeEquals(const std::string& a, const std::string& b) {
    unsigned char diff = (a.size() != b.size()) ? 1 : 0;
    const std::size_t n = std::max(a.size(), b.size());
    for (std::size_t i = 0; i < n; ++i) {
        const unsigned char x = i < a.size() ? static_cast<unsigned char>(a[i]) : 0;
        const unsigned char y = i < b.size() ? static_cast<unsigned char>(b[i]) : 0;
        diff |= static_cast<unsigned char>(x ^ y);
    }
    return diff == 0;
}

std::string trim(const std::string& s) {
    const char* whitespace = " \t\n\r\v\f";
    const std::size_t first = s.find_first_not_of(whitespace);
    if (first == std::string::npos) return "";
    const std::size_t last = s.find_last_not_of(whitespace);
    return s.substr(first, last - first + 1);
}

// Whole-string integer parse ("5" ok, "5x" and "" not ok).
bool parseInt(const std::string& text, int& out) {
    if (text.empty()) return false;
    errno = 0;
    char* end = nullptr;
    const long value = std::strtol(text.c_str(), &end, 10);
    if (errno != 0 || end == text.c_str() || *end != '\0') return false;
    if (value < -1000000 || value > 1000000) return false;
    out = static_cast<int>(value);
    return true;
}

// Reads body[key] as an integer inside [low, high]. JSON booleans and decimals are rejected.
bool readInt(const json& body, const char* key, long long low, long long high, int& out,
             std::string& error) {
    const auto it = body.find(key);
    if (it == body.end() || !it->is_number_integer()) {
        error = std::string("'") + key + "' must be an integer";
        return false;
    }
    long long value = 0;
    if (it->is_number_unsigned()) {
        const auto big = it->get<unsigned long long>();
        value = big > static_cast<unsigned long long>(high) ? high + 1 : static_cast<long long>(big);
    } else {
        value = it->get<long long>();
    }
    if (value < low || value > high) {
        error = std::string("'") + key + "' must be between " + std::to_string(low) + " and " +
                std::to_string(high);
        return false;
    }
    out = static_cast<int>(value);
    return true;
}

// Checks the JSON of POST /scores. On failure 'error' holds a message for the client.
bool validateScore(const json& body, NewScore& out, std::string& error) {
    if (body.is_discarded() || !body.is_object()) {
        error = "JSON object expected";
        return false;
    }
    const auto nameIt = body.find("name");
    if (nameIt == body.end() || !nameIt->is_string()) {
        error = "'name' must be a string";
        return false;
    }
    const std::string name = trim(nameIt->get<std::string>());
    if (name.empty() || name.size() > kNameMax) {
        error = "'name' must be 1 to " + std::to_string(kNameMax) + " characters";
        return false;
    }
    for (unsigned char ch : name) {
        if (ch < 32 || ch >= 127) {
            error = "'name' may only contain printable ASCII characters";
            return false;
        }
    }
    NewScore s;
    s.name = name;
    if (!readInt(body, "score", 0, kScoreMax, s.score, error)) return false;
    if (!readInt(body, "level", 1, 99, s.level, error)) return false;
    if (!readInt(body, "lines", 0, 9999, s.lines, error)) return false;
    out = std::move(s);
    return true;
}

}  // namespace

ApiServer::ApiServer(Settings settings, Database& database)
    : settings_(std::move(settings)), db_(database), server_(std::make_unique<httplib::Server>()) {
    registerRoutes();
}

ApiServer::~ApiServer() = default;

bool ApiServer::run() {
    if (!server_->bind_to_port("127.0.0.1", settings_.serverPort)) return false;
    std::cout << "Leaderboard API listening on http://127.0.0.1:" << settings_.serverPort
              << "  (Ctrl+C to stop)" << std::endl;
    return server_->listen_after_bind();
}

void ApiServer::registerRoutes() {
    server_->set_payload_max_length(kMaxBodyBytes);

    // The library's default on Linux (SO_REUSEPORT) lets a SECOND copy of the server bind the
    // same port without any error. Use the normal behaviour instead: if the port is taken,
    // bind fails and run() reports it.
    server_->set_socket_options([](auto sock) {   // type of sock differs per platform
        int yes = 1;
#ifdef _WIN32
        setsockopt(sock, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&yes),
                   sizeof(yes));
#else
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
#endif
    });

    // One log line per request: method, path, status. Headers and bodies are never logged,
    // so the API key cannot leak into the log.
    server_->set_logger([](const httplib::Request& req, const httplib::Response& res) {
        std::cout << req.method << ' ' << req.path << " -> " << res.status << std::endl;
    });

    // Errors produced by the library itself (unknown URL, body too large) get a JSON body too.
    server_->set_error_handler([](const httplib::Request&, httplib::Response& res) {
        if (!res.body.empty()) return;   // our own handlers already wrote a message
        const char* message = res.status == 404   ? "not found"
                              : res.status == 405 ? "method not allowed"
                              : res.status == 413 ? "request body too large"
                                                  : "request failed";
        sendError(res, res.status, message);
    });

    server_->set_exception_handler(
        [](const httplib::Request&, httplib::Response& res, std::exception_ptr) {
            sendError(res, 500, "internal error");
        });

    server_->Get("/health", [this](const httplib::Request&, httplib::Response& res) {
        if (db_.isReachable())
            sendJson(res, 200, json{{"status", "ok"}, {"database", "up"}});
        else
            sendJson(res, 503, json{{"status", "degraded"}, {"database", "down"}});
    });

    server_->Get("/scores", [this](const httplib::Request& req, httplib::Response& res) {
        int limit = kDefaultLimit;
        if (req.has_param("limit") && !parseInt(req.get_param_value("limit"), limit)) {
            sendError(res, 400, "'limit' must be an integer");
            return;
        }
        limit = std::max(1, std::min(limit, kMaxLimit));

        std::vector<ScoreRow> rows;
        try {
            rows = db_.topScores(limit);
        } catch (const DbError& ex) {
            std::cerr << "database error while reading scores: " << ex.what() << std::endl;
            sendError(res, 503, "database unavailable");
            return;
        }

        json list = json::array();
        for (const ScoreRow& r : rows)
            list.push_back(json{{"name", r.name},
                                {"score", r.score},
                                {"level", r.level},
                                {"lines", r.lines},
                                {"created_at", r.createdAt}});
        sendJson(res, 200, json{{"scores", list}});
    });

    server_->Post("/scores", [this](const httplib::Request& req, httplib::Response& res) {
        if (!constantTimeEquals(req.get_header_value("X-API-Key"), settings_.apiKey)) {
            sendError(res, 401, "missing or invalid API key");
            return;
        }

        const json body = json::parse(req.body, nullptr, /*allow_exceptions=*/false);
        NewScore score;
        std::string error;
        if (!validateScore(body, score, error)) {
            sendError(res, 400, error);
            return;
        }

        try {
            const unsigned long long id = db_.insertScore(score);
            sendJson(res, 201, json{{"id", id}});
        } catch (const DbError& ex) {
            std::cerr << "database error while saving a score: " << ex.what() << std::endl;
            sendError(res, 503, "database unavailable");
        }
    });
}
