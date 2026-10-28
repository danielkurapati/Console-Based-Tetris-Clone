#!/usr/bin/env bash
# Run ONCE on Ubuntu / Debian (also Kali, Mint). Installs the C++ compiler and curl.
# No database server is needed: the leaderboard uses SQLite, which is built in.
set -euo pipefail

if ! command -v apt-get >/dev/null 2>&1; then
    echo "This script is for Ubuntu/Debian (apt). Install g++, gcc and curl manually."
    exit 1
fi

echo "Installing packages (sudo password may be asked)..."
sudo apt-get update
sudo apt-get install -y g++ gcc curl

echo
echo "Done. Next: ./build.sh   then   ./start_all.sh"
