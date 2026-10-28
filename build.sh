#!/usr/bin/env bash
# Builds ./tetris (game) and ./tetris_server (REST server). Run after every code change.
set -euo pipefail
cd "$(dirname "$0")"

command -v g++ >/dev/null 2>&1 || { echo "g++ not found. Run ./install_deps.sh first."; exit 1; }
command -v gcc >/dev/null 2>&1 || { echo "gcc not found. Run ./install_deps.sh first."; exit 1; }

echo "[1/3] Building the game: tetris"
rm -f tetris
g++ -std=c++17 -O2 -Wall -Wextra src/*.cpp -o tetris -pthread

# SQLite is one big C file. It takes about 30 seconds, so it is compiled only once.
mkdir -p build
if [ ! -f build/sqlite3.o ] || [ server/third_party/sqlite3.c -nt build/sqlite3.o ]; then
    echo "[2/3] Compiling SQLite (first time only, about 30 seconds)..."
    gcc -O2 -c -DSQLITE_THREADSAFE=1 -DSQLITE_OMIT_LOAD_EXTENSION \
        server/third_party/sqlite3.c -o build/sqlite3.o
else
    echo "[2/3] SQLite already compiled."
fi

echo "[3/3] Building the server: tetris_server"
rm -f tetris_server
g++ -std=c++17 -O2 -Wall -Wextra server/*.cpp build/sqlite3.o -o tetris_server -pthread

echo
echo "Build finished: ./tetris and ./tetris_server"
