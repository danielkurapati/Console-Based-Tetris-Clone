// All settings and secrets come from environment variables

#include <exception>
#include <iostream>

#include "ApiServer.h"
#include "Database.h"
#include "Settings.h"

int main() {
    try {
        const Settings settings = Settings::fromEnvironment();

        Database database(DbSettings{settings.dbHost, settings.dbPort, settings.dbUser,
                                     settings.dbPassword, settings.dbName});
        database.initSchema();   // fails fast if the database cannot be reached

        ApiServer server(settings, database);
        if (!server.run()) {
            std::cerr << "ERROR: cannot listen on port " << settings.serverPort
                      << " (is another program using it? set PORT to use a different one)\n";
            return 1;
        }
    } catch (const DbError& ex) {
        std::cerr << "ERROR: cannot use the database: " << ex.what() << "\n"
                  << "Check that MySQL is running and that the DATABASE_* variables are right.\n";
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
