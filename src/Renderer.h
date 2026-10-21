#pragma once
// IRenderer: everything the game needs to draw. States only know this interface,
// so the console drawing can be replaced (e.g. by a GUI) without touching game logic.

#include <string>

#include "Board.h"
#include "ScoreManager.h"
#include "ScoreService.h"
#include "Tetromino.h"

class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual void beginFrame() = 0;
    virtual void drawBoard(const Board& board) = 0;
    virtual void drawPiece(const Tetromino& piece) = 0;
    virtual void drawNext(const Tetromino& piece) = 0;
    virtual void drawHUD(const ScoreManager& score) = 0;
    virtual void drawLeaderboard(const ScoreBoard& board) = 0;
    virtual void drawOverlay(const std::string& title, const std::string& subtitle) = 0;
    virtual void endFrame() = 0;
};

