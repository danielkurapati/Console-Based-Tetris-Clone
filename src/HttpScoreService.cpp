#include "HttpScoreService.h"

#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
#endif

#include "third_party/httplib.h"
#include "third_party/json.hpp"

namespace {

constexpr int kTimeoutSeconds = 2;   // never wait longer than this for the server

void configure(httplib::Client& client) {
    client.set_connection_timeout(kTimeoutSeconds, 0);
    client.set_read_timeout(kTimeoutSeconds, 0);
    client.set_write_timeout(kTimeoutSeconds, 0);
}

}

HttpScoreService::HttpScoreService(std::string baseUrl, std::string apiKey, int boardSize)
    : baseUrl_(std::move(baseUrl)),
      apiKey_(std::move(apiKey)),
      boardSize_(boardSize),
      worker_([this] { workerLoop(); }) {
    board_.status = ServiceStatus::Loading;
}

HttpScoreService::~HttpScoreService() {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        stop_ = true;                    // tasks that have not started yet are dropped
    }
    queueCv_.notify_all();
    worker_.join();                      // waits for the request in progress (max ~2 s)
}

void HttpScoreService::submit(const ScoreEntry& entry) {
    setStatus(ServiceStatus::Loading, "");
    enqueue([this, entry] { doSubmit(entry); });
}

void HttpScoreService::refresh() {
    setStatus(ServiceStatus::Loading, "");
    enqueue([this] { doRefresh(); });
}

ScoreBoard HttpScoreService::board() const {
    std::lock_guard<std::mutex> lock(boardMutex_);
    return board_;                       // a copy: safe to use while the worker keeps going
}

void HttpScoreService::enqueue(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        queue_.push_back(std::move(task));
    }
    queueCv_.notify_one();
}

void HttpScoreService::workerLoop() {
    for (;;) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCv_.wait(lock, [this] { return stop_ || !queue_.empty(); });
            if (stop_) return;
            task = std::move(queue_.front());
            queue_.pop_front();
        }
        task();                          // network work happens outside the lock
    }
}

void HttpScoreService::setStatus(ServiceStatus status, const std::string& message) {
    std::lock_guard<std::mutex> lock(boardMutex_);
    board_.status = status;
    board_.message = message;
}

void HttpScoreService::doSubmit(const ScoreEntry& entry) {
    httplib::Client client(baseUrl_);
    configure(client);

    nlohmann::json body = {{"name", entry.name},
                           {"score", entry.score},
                           {"level", entry.level},
                           {"lines", entry.lines}};
    httplib::Headers headers = {{"X-API-Key", apiKey_}};

    auto res = client.Post("/scores", headers, body.dump(), "application/json");
    if (!res) {
        setStatus(ServiceStatus::Offline, "Server offline");
        return;
    }
    if (res->status != 201) {
        setStatus(ServiceStatus::Offline, "Score rejected (HTTP " + std::to_string(res->status) + ")");
        return;
    }
    doRefresh();                         // show the new ranking
}

void HttpScoreService::doRefresh() {
    httplib::Client client(baseUrl_);
    configure(client);

    auto res = client.Get("/scores?limit=" + std::to_string(boardSize_));
    if (!res) {
        setStatus(ServiceStatus::Offline, "Server offline");
        return;
    }
    if (res->status != 200) {
        setStatus(ServiceStatus::Offline, "Server error (HTTP " + std::to_string(res->status) + ")");
        return;
    }

    std::vector<ScoreEntry> entries;
    try {
        const nlohmann::json reply = nlohmann::json::parse(res->body);
        for (const auto& item : reply.at("scores")) {
            ScoreEntry e;
            e.name = cleanName(item.value("name", std::string()));
            e.score = item.value("score", 0);
            e.level = item.value("level", 1);
            e.lines = item.value("lines", 0);
            entries.push_back(std::move(e));
        }
    } catch (const std::exception&) {
        setStatus(ServiceStatus::Offline, "Bad server reply");
        return;
    }

    std::lock_guard<std::mutex> lock(boardMutex_);
    board_.entries = std::move(entries);
    board_.status = ServiceStatus::Ready;
    board_.message.clear();
}
