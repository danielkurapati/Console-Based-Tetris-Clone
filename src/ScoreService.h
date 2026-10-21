#pragma once

#include <string>
#include <vector>

struct ScoreEntry {
    std::string name;
    int score = 0;
    int level = 1;
    int lines = 0;
};

enum class ServiceStatus {
    Disabled,   
    Loading,    
    Ready,      
    Offline     
};

// A snapshot of what the leaderboard currently looks like.
struct ScoreBoard {
    ServiceStatus status = ServiceStatus::Disabled;
    std::vector<ScoreEntry> entries;   // best first
    std::string message;               // short text for the player when status is Offline
};

class IScoreService {
public:
    virtual ~IScoreService() = default;

    virtual void submit(const ScoreEntry& entry) = 0;   // send a score, then reload the board
    virtual void refresh() = 0;                         // reload the board
    virtual ScoreBoard board() const = 0;               // current snapshot (thread-safe)
};

// Used when no leaderboard is configured.
class NullScoreService : public IScoreService {
public:
    void submit(const ScoreEntry&) override {}
    void refresh() override {}
    ScoreBoard board() const override { return ScoreBoard{}; }
};

// Keeps printable ASCII only, trims spaces, limits the length. Never returns an empty name.
inline std::string cleanName(const std::string& raw, std::size_t maxLen = 20) {
    std::string out;
    for (char ch : raw)
        if (ch >= 32 && ch < 127) out += ch;
    std::size_t first = out.find_first_not_of(' ');
    if (first == std::string::npos) return "Player";
    std::size_t last = out.find_last_not_of(' ');
    out = out.substr(first, last - first + 1);
    if (out.size() > maxLen) out.resize(maxLen);
    return out;
}

