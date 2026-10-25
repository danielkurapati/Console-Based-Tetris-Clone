#pragma once
// HttpScoreService: IScoreService implemented with the leaderboard REST API.
//
//   POST {base}/scores            body {"name","score","level","lines"}, header X-API-Key
//   GET  {base}/scores?limit=N    reply {"scores":[{"name","score","level","lines",...}]}
//
// One background worker thread runs the HTTP requests one after another, so the game loop
// never waits for the network.

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include "ScoreService.h"

class HttpScoreService : public IScoreService {
public:
    HttpScoreService(std::string baseUrl, std::string apiKey, int boardSize = 8);
    ~HttpScoreService() override;
    HttpScoreService(const HttpScoreService&) = delete;
    HttpScoreService& operator=(const HttpScoreService&) = delete;

    void submit(const ScoreEntry& entry) override;
    void refresh() override;
    ScoreBoard board() const override;

private:
    void enqueue(std::function<void()> task);
    void workerLoop();
    void doSubmit(const ScoreEntry& entry);
    void doRefresh();
    void setStatus(ServiceStatus status, const std::string& message);

    std::string baseUrl_;
    std::string apiKey_;
    int boardSize_;

    mutable std::mutex boardMutex_;      // protects board_
    ScoreBoard board_;

    std::mutex queueMutex_;              // protects queue_ and stop_
    std::condition_variable queueCv_;
    std::deque<std::function<void()>> queue_;
    bool stop_ = false;

    std::thread worker_;                 // declared last: starts after everything above exists
};
