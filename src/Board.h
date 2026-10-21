#pragma once

#include "Config.h"
#include "Matrix.h"
#include "Tetromino.h"

class Board {
public:
    Board() : grid_(ROWS, COLS, 0) {}

    int at(int r, int c) const { return grid_[r][c]; }   

    // True if every cell of the piece is inside the board and on an empty cell.
    bool isValid(const Tetromino& t) const {
        for (const auto& cell : t.cells()) {
            int x = cell.first, y = cell.second;
            if (x < 0 || x >= COLS || y < 0 || y >= ROWS) return false;
            if (grid_[y][x] != 0) return false;
        }
        return true;
    }

    void lock(const Tetromino& t) {
        for (const auto& cell : t.cells())
            grid_[cell.second][cell.first] = t.type();
    }

    // Removes all full rows, drops everything above, returns how many were cleared.
    int clearFullLines() {
        int cleared = 0;
        for (int r = ROWS - 1; r >= 0;) {
            if (isRowFull(r)) {
                removeRow(r);
                ++cleared;          
            } else {
                --r;
            }
        }
        return cleared;
    }

    void reset() { grid_.fill(0); }

private:
    bool isRowFull(int r) const {
        for (int c = 0; c < COLS; ++c)
            if (grid_[r][c] == 0) return false;
        return true;
    }

    void removeRow(int r) {
        for (int rr = r; rr > 0; --rr)
            for (int c = 0; c < COLS; ++c)
                grid_[rr][c] = grid_[rr - 1][c];
        for (int c = 0; c < COLS; ++c) grid_[0][c] = 0;
    }

    Matrix<int> grid_;
};
