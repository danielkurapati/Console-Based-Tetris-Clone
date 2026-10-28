// Leaderboard REST API server (C++).
// All settings and secrets come from environment variables - see Settings.h.

#include <exception>
#include <iostream>

#include "ApiServer.h"
#include "Database.h"
#include "Settings.h"

int main() {
    try {
        const Settings settings = Settings::fromEnvironment();

        Database database(DbSettings{settings.dbFile});
        database.initSchema();   // creates the file and table; fails fast if that is impossible

        ApiServer server(settings, database);
        if (!server.run()) {
            std::cerr << "ERROR: cannot listen on port " << settings.serverPort
                      << " (is another program using it? set PORT to use a different one)\n";
            return 1;
        }
    } catch (const DbError& ex) {
        std::cerr << "ERROR: cannot use the database: " << ex.what() << "\n"
                  << "Check that the folder is writable, or set DATABASE_FILE to another path.\n";
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
