#pragma once
// Settings: everything the server needs, read from ENVIRONMENT VARIABLES.
// No password or key is ever written in a source file.
//
//   DATABASE_PASSWORD   required   password of the database account
//   API_KEY             required   key that clients must send in the X-API-Key header
//   DATABASE_HOST       optional   default 127.0.0.1
//   DATABASE_PORT       optional   default 3306
//   DATABASE_USER       optional   default tetris
//   DATABASE_NAME       optional   default tetris
//   PORT                optional   port this server listens on, default 5000

#include <stdexcept>
#include <string>

#include "../src/Env.h"   // getEnv(): wrapper around std::getenv

class Settings {
public:
    std::string dbHost;
    unsigned int dbPort = 3306;
    std::string dbUser;
    std::string dbPassword;
    std::string dbName;
    std::string apiKey;
    int serverPort = 5000;

    // Throws std::runtime_error with a helpful message if a required variable is missing.
    static Settings fromEnvironment() {
        Settings s;
        s.dbHost = getEnv("DATABASE_HOST", "127.0.0.1");
        s.dbPort = static_cast<unsigned int>(readPort("DATABASE_PORT", 3306));
        s.dbUser = getEnv("DATABASE_USER", "tetris");
        s.dbPassword = requireEnv("DATABASE_PASSWORD");
        s.dbName = getEnv("DATABASE_NAME", "tetris");
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
