#include "Game.h"

#include <chrono>
#include <thread>

Game::Game(IScoreService& scores, std::string playerName) {
    ctx_.states = &states_;
    ctx_.scores = &scores;
    ctx_.playerName = std::move(playerName);
    ctx_.scores->refresh();      // load the leaderboard in the background for the menu screen
    ctx_.reset();
    states_.changeTo<MenuState>();
    states_.applyPending();
}

void Game::run() {
    using Clock = std::chrono::steady_clock;
    const auto frameTime = std::chrono::milliseconds(33);   // about 30 frames per second
    auto lastTick = Clock::now();

    while (running_) {
        const auto frameStart = Clock::now();

        // 1. keyboard -> events
        input_.poll(terminal_, events_);

        // 2. gravity timer -> Tick event
        if (frameStart - lastTick >= ctx_.score.dropInterval()) {
            events_.push(Event{EventType::Tick});
            lastTick = frameStart;
        }

        // 3. dispatch events to the current state (through the proxy)
        Event e;
        while (events_.poll(e)) {
            if (e.type == EventType::Quit) {
                running_ = false;
                break;
            }
            states_.handleEvent(e, ctx_);
            states_.applyPending();          // so the next event already sees the new state
        }
        if (!running_) break;

        // 4. update + draw
        states_.update(ctx_);
        renderer_.beginFrame();
        states_.render(renderer_, ctx_);
        renderer_.endFrame();

        // 5. keep a steady frame rate
        std::this_thread::sleep_until(frameStart + frameTime);
    }
}
