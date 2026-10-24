#pragma once

#include <string>

#include "Config.h"
#include "Matrix.h"
#include "Renderer.h"

class ConsoleRenderer : public IRenderer {
public:
    ConsoleRenderer() : field_(ROWS, COLS, 0) {}

    void beginFrame() override;
    void drawBoard(const Board& board) override;
    void drawPiece(const Tetromino& piece) override;
    void drawNext(const Tetromino& piece) override;
    void drawHUD(const ScoreManager& score) override;
    void drawLeaderboard(const ScoreBoard& board) override;
    void drawOverlay(const std::string& title, const std::string& subtitle) override;
    void endFrame() override;

private:
    std::string panelLine(int row) const;   // text shown to the right of board row 'row'
    std::string leaderboardLine(int i) const;   // i-th line of the leaderboard block

    Matrix<int> field_;       // colors of the board plus the falling piece
    Tetromino next_;
    bool hasNext_ = false;
    bool hasHud_ = false;
    int score_ = 0, level_ = 1, lines_ = 0;
    bool overlay_ = false;
    ScoreBoard board_;
    bool hasBoard_ = false;
    std::string title_, subtitle_;
};
