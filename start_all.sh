#!/usr/bin/env bash
# Starts the leaderboard server in the background, then the game.
# The API key is typed here (hidden) and lives only in this session's environment
# variables. It is never written to a file.
set -uo pipefail
cd "$(dirname "$0")"

if [ ! -x ./tetris ] || [ ! -x ./tetris_server ]; then
    echo "tetris or tetris_server is missing. Run ./build.sh first."
    exit 1
fi

# Asked only when the variable is not already set in this terminal.
if [ -z "${API_KEY:-}" ]; then
    read -rsp 'Choose an API key (any secret text): ' API_KEY
    echo
fi
export API_KEY
# The game must use the same key as the server.
export SCORE_API_KEY="$API_KEY"

PORT="${PORT:-5000}"
export PORT
export SCORE_API_URL="${SCORE_API_URL:-http://127.0.0.1:$PORT}"

SERVER_PID=""
stop_server() {
    if [ -n "$SERVER_PID" ] && kill -0 "$SERVER_PID" 2>/dev/null; then
        kill "$SERVER_PID" 2>/dev/null
        wait "$SERVER_PID" 2>/dev/null
    fi
}
trap stop_server EXIT

echo "Starting the leaderboard server (log: server.log)..."
./tetris_server >server.log 2>&1 &
SERVER_PID=$!

# Wait up to ~10 s until the server answers /health, or stop early if it died.
ready=0
for _ in $(seq 1 20); do
    if ! kill -0 "$SERVER_PID" 2>/dev/null; then
        echo "The server stopped. Its log says:"
        cat server.log
        exit 1
    fi
    if command -v curl >/dev/null 2>&1; then
        curl -fs "$SCORE_API_URL/health" >/dev/null 2>&1 && { ready=1; break; }
    elif grep -q "listening" server.log 2>/dev/null; then
        ready=1; break
    fi
    sleep 0.5
done
if [ "$ready" -ne 1 ]; then
    echo "The server did not become ready in time. Log:"
    cat server.log
    exit 1
fi

./tetris

echo
echo "Game closed. Stopping the leaderboard server."
