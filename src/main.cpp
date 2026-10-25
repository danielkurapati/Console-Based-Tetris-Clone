#include <exception>
#include <iostream>
#include <memory>
#include <string>

#include "Env.h"
#include "Game.h"
#include "HttpScoreService.h"
#include "ScoreService.h"

int main() {
    try {
        // Settings come from environment variables, not from the source code.
        const std::string apiKey = getEnv("SCORE_API_KEY");
        const std::string apiUrl = getEnv("SCORE_API_URL", "http://127.0.0.1:5000");

        std::unique_ptr<IScoreService> scores;   // declared before 'game': destroyed after it
        std::string player = "Player";

        if (!apiKey.empty()) {
            std::cout << "Online leaderboard: " << apiUrl << "\n"
                      << "Enter your name (max 20 characters): " << std::flush;
            std::string typed;
            std::getline(std::cin, typed);
            player = cleanName(typed);
            scores = std::make_unique<HttpScoreService>(apiUrl, apiKey);
        } else {
            std::cout << "SCORE_API_KEY is not set, so the game runs without the online leaderboard.\n";
            scores = std::make_unique<NullScoreService>();
        }

        Game game(*scores, player);
        game.run();
    } catch (const std::exception& ex) {
        // The Game (and its Terminal) is already destroyed here, so the screen is restored.
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
