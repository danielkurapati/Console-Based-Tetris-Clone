#include "ConsoleRenderer.h"

#include "Terminal.h"

namespace {

const char* colorCode(int type) {
    switch (type) {
        case 1: return "\x1b[46m";         // I  cyan
        case 2: return "\x1b[43m";         // O  yellow
        case 3: return "\x1b[45m";         // T  magenta
        case 4: return "\x1b[42m";         // S  green
        case 5: return "\x1b[41m";         // Z  red
        case 6: return "\x1b[44m";         // J  blue
        case 7: return "\x1b[48;5;208m";   // L  orange
        default: return "\x1b[47m";
    }
}

// One board cell = 2 characters.
std::string cellStr(int type) {
    if (type == 0) return "\x1b[90m .\x1b[0m";                 // empty: dim dot
    return std::string(colorCode(type)) + "  \x1b[0m";          // filled: colored block
}

std::string printable(const std::string& s, std::size_t maxLen) {
    std::string out;
    for (char ch : s) {
        if (out.size() >= maxLen) break;
        out += (ch >= 32 && ch < 127) ? ch : '?';
    }
    return out;
}

std::string padRight(const std::string& s, std::size_t width) {
    return s.size() >= width ? s : s + std::string(width - s.size(), ' ');
}

std::string padLeft(const std::string& s, std::size_t width) {
    return s.size() >= width ? s : std::string(width - s.size(), ' ') + s;
}

std::string center(const std::string& s, int width) {
    std::string t = s.substr(0, static_cast<std::size_t>(width));
    int len = static_cast<int>(t.size());
    int left = (width - len) / 2;
    int right = width - len - left;
    return std::string(static_cast<std::size_t>(left), ' ') + t +
           std::string(static_cast<std::size_t>(right), ' ');
}

}  // namespace

void ConsoleRenderer::beginFrame() {
    field_.fill(0);
    hasNext_ = false;
    hasHud_ = false;
    overlay_ = false;
    hasBoard_ = false;
}

void ConsoleRenderer::drawBoard(const Board& board) {
    for (int r = 0; r < ROWS; ++r)
        for (int c = 0; c < COLS; ++c)
            field_[r][c] = board.at(r, c);
}

void ConsoleRenderer::drawPiece(const Tetromino& piece) {
    for (const auto& cell : piece.cells())
        if (field_.inBounds(cell.second, cell.first))
            field_[cell.second][cell.first] = piece.type();
}

void ConsoleRenderer::drawNext(const Tetromino& piece) {
    next_ = piece;
    hasNext_ = true;
}

void ConsoleRenderer::drawHUD(const ScoreManager& s) {
    score_ = s.score();
    level_ = s.level();
    lines_ = s.lines();
    hasHud_ = true;
}

void ConsoleRenderer::drawLeaderboard(const ScoreBoard& b) {
    board_ = b;
    hasBoard_ = (b.status != ServiceStatus::Disabled);   // offline mode: keep showing the controls
}

void ConsoleRenderer::drawOverlay(const std::string& title, const std::string& subtitle) {
    title_ = title;
    subtitle_ = subtitle;
    overlay_ = true;
}

std::string ConsoleRenderer::panelLine(int r) const {
    const std::string pad = "   ";
    if (r == 0) return pad + "NEXT";
    if (r >= 1 && r <= 4) {
        if (!hasNext_) return "";
        int rr = r - 1;
        std::string s = pad;
        if (rr < next_.shape().rows())
            for (int c = 0; c < next_.shape().cols(); ++c)
                s += (next_.shape()[rr][c] != 0) ? cellStr(next_.type()) : std::string("  ");
        return s;
    }
    if (hasHud_) {
        if (r == 6) return pad + "Score: " + std::to_string(score_);
        if (r == 7) return pad + "Level: " + std::to_string(level_);
        if (r == 8) return pad + "Lines: " + std::to_string(lines_);
    }
    if (hasBoard_ && r >= 10) return leaderboardLine(r - 10);

    switch (r) {
        case 10: return pad + "Controls";
        case 11: return pad + "Left/Right or A/D  move";
        case 12: return pad + "Up or W            rotate";
        case 13: return pad + "Down or S          soft drop";
        case 14: return pad + "Space              hard drop";
        case 15: return pad + "P                  pause";
        case 16: return pad + "Q or Esc           quit";
        default: return "";
    }
}

// Lines 0..9 of the block under the score: header, up to 8 entries, one status line.
std::string ConsoleRenderer::leaderboardLine(int i) const {
    const std::string pad = "   ";
    const int maxRows = 8;
    const int n = static_cast<int>(board_.entries.size());

    if (i == 0) return pad + "LEADERBOARD";

    const std::string statusText =
        board_.status == ServiceStatus::Loading ? std::string("updating...")
        : board_.status == ServiceStatus::Offline
            ? printable(board_.message.empty() ? std::string("Server offline") : board_.message, 28)
            : std::string();

    if (n == 0) {
        if (i != 1) return "";
        if (board_.status == ServiceStatus::Loading) return pad + "Loading...";
        if (board_.status == ServiceStatus::Offline) return pad + statusText;
        return pad + "No scores yet";
    }

    if (i >= 1 && i <= maxRows && i <= n) {
        const ScoreEntry& e = board_.entries[static_cast<std::size_t>(i - 1)];
        return pad + padLeft(std::to_string(i), 2) + ". " + padRight(printable(e.name, 12), 12) +
               padLeft(std::to_string(e.score), 8);
    }
    if (i == maxRows + 1 && !statusText.empty()) return pad + statusText;
    return "";
}

void ConsoleRenderer::endFrame() {
    const std::string border = "+" + std::string(COLS * 2, '-') + "+";

    std::string out = "\x1b[H";                         // cursor to top-left
    out += "  CONSOLE TETRIS\x1b[K\n";
    out += border + "\x1b[K\n";

    for (int r = 0; r < ROWS; ++r) {
        out += '|';
        if (overlay_ && r == ROWS / 2 - 1) {
            out += "\x1b[1;97m" + center(title_, COLS * 2) + "\x1b[0m";
        } else if (overlay_ && r == ROWS / 2) {
            out += "\x1b[97m" + center(subtitle_, COLS * 2) + "\x1b[0m";
        } else {
            for (int c = 0; c < COLS; ++c) out += cellStr(field_[r][c]);
        }
        out += '|';
        out += panelLine(r);
        out += "\x1b[K\n";                              // clear rest of line
    }
    out += border + "\x1b[K";

    Terminal::write(out);
}

