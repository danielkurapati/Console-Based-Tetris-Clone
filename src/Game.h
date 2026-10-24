#pragma once

#include <string>

#include "ConsoleRenderer.h"
#include "Event.h"
#include "InputHandler.h"
#include "ScoreService.h"
#include "States.h"
#include "Terminal.h"

class Game {
public:
    // 'scores' must outlive the Game (main() owns it).
    Game(IScoreService& scores, std::string playerName);
    void run();

private:
    Terminal terminal_;
    InputHandler input_;
    EventQueue events_;
    GameContext ctx_;
    StateProxy states_;
    ConsoleRenderer renderer_;
    bool running_ = true;
};
