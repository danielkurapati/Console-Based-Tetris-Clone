#pragma once

#include <memory>
#include <string>

#include "Board.h"
#include "Event.h"
#include "Renderer.h"
#include "ScoreManager.h"
#include "ScoreService.h"
#include "Tetromino.h"

class StateProxy;

struct GameContext {
    Board board;
    TetrominoFactory factory;
    Tetromino current;
    Tetromino next;
    ScoreManager score;
    StateProxy* states = nullptr;
    IScoreService* scores = nullptr;     // online leaderboard (REST API); may be a do-nothing one
    std::string playerName = "Player";

    // Start a fresh game.
    void reset() {
        board.reset();
        score.reset();
        current = factory.next();
        next = factory.next();
    }

    // Called once when a game ends: send the score to the leaderboard (non-blocking).
    void finishGame() {
        if (scores == nullptr) return;
        if (score.score() > 0)
            scores->submit(ScoreEntry{playerName, score.score(), score.level(), score.lines()});
        else
            scores->refresh();
    }

    // Falling piece -> the preview piece; draw a new preview.
    // Returns false if the new piece does not fit (= game over).
    bool spawnNext() {
        current = next;
        next = factory.next();
        return board.isValid(current);
    }
};

class GameState {
public:
    virtual ~GameState() = default;
    virtual void handleEvent(const Event& e, GameContext& ctx) = 0;
    virtual void update(GameContext&) {}
    virtual void render(IRenderer& r, const GameContext& ctx) = 0;
};

class StateProxy {
public:
    // Request a state change. It takes effect at applyPending(), never in the middle
    // of a state's own function (which would destroy the object that is running).
    void change(std::unique_ptr<GameState> s) { pending_ = std::move(s); }

    template <class S>
    void changeTo() { change(std::make_unique<S>()); }

    void applyPending() {
        if (pending_) current_ = std::move(pending_);
    }

    void handleEvent(const Event& e, GameContext& ctx) { if (current_) current_->handleEvent(e, ctx); }
    void update(GameContext& ctx)                       { if (current_) current_->update(ctx); }
    void render(IRenderer& r, const GameContext& ctx)   { if (current_) current_->render(r, ctx); }

private:
    std::unique_ptr<GameState> current_;
    std::unique_ptr<GameState> pending_;
};


class MenuState : public GameState {
public:
    void handleEvent(const Event& e, GameContext& ctx) override;
    void render(IRenderer& r, const GameContext& ctx) override;
};

class PlayingState : public GameState {
public:
    void handleEvent(const Event& e, GameContext& ctx) override;
    void render(IRenderer& r, const GameContext& ctx) override;

private:
    bool tryMove(GameContext& ctx, int dx, int dy);
    bool tryRotate(GameContext& ctx);
    void gravityStep(GameContext& ctx);
    void hardDrop(GameContext& ctx);
    void lockPiece(GameContext& ctx);
};

class PausedState : public GameState {
public:
    void handleEvent(const Event& e, GameContext& ctx) override;
    void render(IRenderer& r, const GameContext& ctx) override;
};

class GameOverState : public GameState {
public:
    void handleEvent(const Event& e, GameContext& ctx) override;
    void render(IRenderer& r, const GameContext& ctx) override;
};
