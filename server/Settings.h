#pragma once
// Settings: everything the server needs, read from ENVIRONMENT VARIABLES.
// No password or key is ever written in a source file.
//
//   API_KEY             required   key that clients must send in the X-API-Key header
//   DATABASE_FILE       optional   path of the SQLite database file, default tetris.db
//   PORT                optional   port this server listens on, default 5000

#include <stdexcept>
#include <string>

#include "../src/Env.h"   // getEnv(): wrapper around std::getenv

class Settings {
public:
    std::string dbFile;
    std::string apiKey;
    int serverPort = 5000;

    // Throws std::runtime_error with a helpful message if a required variable is missing.
    static Settings fromEnvironment() {
        Settings s;
        s.dbFile = getEnv("DATABASE_FILE", "tetris.db");
        s.apiKey = requireEnv("API_KEY");
        s.serverPort = readPort("PORT", 5000);
        return s;
    }

private:
    static std::string requireEnv(const char* name) {
        std::string value = getEnv(name);
        if (value.empty())
            throw std::runtime_error(std::string("environment variable ") + name +
                                     " is not set.\n  Linux/macOS : export " + name + "=...\n" +
                                     "  PowerShell  : $env:" + name + "=\"...\"");
        return value;
    }

    static int readPort(const char* name, int fallback) {
        std::string text = getEnv(name);
        if (text.empty()) return fallback;
        try {
            std::size_t used = 0;
            int port = std::stoi(text, &used);
            if (used == text.size() && port >= 1 && port <= 65535) return port;
        } catch (const std::exception&) {
        }
        throw std::runtime_error(std::string("environment variable ") + name +
                                 " must be a port number between 1 and 65535");
    }
};
