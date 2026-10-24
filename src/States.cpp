#include "States.h"

//  MenuState 

void MenuState::handleEvent(const Event& e, GameContext& ctx) {
    if (e.type == EventType::Confirm) {
        ctx.reset();
        ctx.states->changeTo<PlayingState>();
    }
}

void MenuState::render(IRenderer& r, const GameContext& ctx) {
    r.drawBoard(ctx.board);
    r.drawNext(ctx.next);
    r.drawHUD(ctx.score);
    if (ctx.scores) r.drawLeaderboard(ctx.scores->board());
    r.drawOverlay("TETRIS", "Enter = start");
}

//  PlayingState 

void PlayingState::handleEvent(const Event& e, GameContext& ctx) {
    switch (e.type) {
        case EventType::MoveLeft:  tryMove(ctx, -1, 0); break;
        case EventType::MoveRight: tryMove(ctx, 1, 0);  break;
        case EventType::RotateCW:  tryRotate(ctx);      break;
        case EventType::SoftDrop:
            if (tryMove(ctx, 0, 1)) ctx.score.onSoftDrop(1);
            else lockPiece(ctx);
            break;
        case EventType::HardDrop:  hardDrop(ctx);       break;
        case EventType::Tick:      gravityStep(ctx);    break;
        case EventType::Pause:     ctx.states->changeTo<PausedState>(); break;
        default: break;
    }
}

// Build a moved copy, accept it only if the board says it is valid.
bool PlayingState::tryMove(GameContext& ctx, int dx, int dy) {
    Tetromino candidate = ctx.current.movedBy(dx, dy);
    if (!ctx.board.isValid(candidate)) return false;
    ctx.current = candidate;
    return true;
}

// Rotate, with a simple "wall kick": if it does not fit, try nudging sideways.
bool PlayingState::tryRotate(GameContext& ctx) {
    Tetromino rotated = ctx.current.rotatedCW();
    static const int kicks[] = {0, -1, 1, -2, 2};
    for (int dx : kicks) {
        Tetromino candidate = rotated.movedBy(dx, 0);
        if (ctx.board.isValid(candidate)) {
            ctx.current = candidate;
            return true;
        }
    }
    return false;
}

void PlayingState::gravityStep(GameContext& ctx) {
    if (!tryMove(ctx, 0, 1)) lockPiece(ctx);
}

void PlayingState::hardDrop(GameContext& ctx) {
    int dropped = 0;
    while (tryMove(ctx, 0, 1)) ++dropped;
    ctx.score.onHardDrop(dropped);
    lockPiece(ctx);
}

void PlayingState::lockPiece(GameContext& ctx) {
    ctx.board.lock(ctx.current);
    int cleared = ctx.board.clearFullLines();
    if (cleared > 0) ctx.score.onLinesCleared(cleared);
    if (!ctx.spawnNext()) {
        ctx.finishGame();                      // REST call: send the score, reload the leaderboard
        ctx.states->changeTo<GameOverState>();
    }
}

void PlayingState::render(IRenderer& r, const GameContext& ctx) {
    r.drawBoard(ctx.board);
    r.drawPiece(ctx.current);
    r.drawNext(ctx.next);
    r.drawHUD(ctx.score);
}

//  PausedState 

void PausedState::handleEvent(const Event& e, GameContext& ctx) {
    if (e.type == EventType::Pause || e.type == EventType::Confirm)
        ctx.states->changeTo<PlayingState>();
}

void PausedState::render(IRenderer& r, const GameContext& ctx) {
    r.drawBoard(ctx.board);
    r.drawPiece(ctx.current);
    r.drawNext(ctx.next);
    r.drawHUD(ctx.score);
    r.drawOverlay("PAUSED", "P = resume");
}

//  GameOverState 

void GameOverState::handleEvent(const Event& e, GameContext& ctx) {
    if (e.type == EventType::Confirm) {
        ctx.reset();
        ctx.states->changeTo<PlayingState>();
    }
}

void GameOverState::render(IRenderer& r, const GameContext& ctx) {
    r.drawBoard(ctx.board);
    r.drawHUD(ctx.score);
    if (ctx.scores) r.drawLeaderboard(ctx.scores->board());
    r.drawOverlay("GAME OVER", "Enter = play again");
}
