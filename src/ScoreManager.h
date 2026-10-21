#pragma once

#include <algorithm>
#include <chrono>

class ScoreManager {
public:
    void onLinesCleared(int n) {
        static const int table[5] = {0, 100, 300, 500, 800};
        if (n < 0) n = 0;
        if (n > 4) n = 4;
        score_ += table[n] * level_;
        lines_ += n;
        level_ = 1 + lines_ / 10;          // level up every 10 lines
    }
    void onSoftDrop(int cells) { score_ += cells; }
    void onHardDrop(int cells) { score_ += 2 * cells; }
    void reset() { score_ = 0; lines_ = 0; level_ = 1; }

    int score() const { return score_; }
    int lines() const { return lines_; }
    int level() const { return level_; }

    std::chrono::milliseconds dropInterval() const {
        int ms = 800 - (level_ - 1) * 70;
        return std::chrono::milliseconds(std::max(100, ms));
    }

private:
    int score_ = 0;
    int lines_ = 0;
    int level_ = 1;
};

