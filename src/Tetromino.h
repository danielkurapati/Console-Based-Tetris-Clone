#pragma once

#include <algorithm>
#include <chrono>
#include <initializer_list>
#include <random>
#include <utility>
#include <vector>

#include "Config.h"
#include "Matrix.h"

class Tetromino {
public:
    Tetromino() : shape_(1, 1, 0), type_(0), x_(0), y_(0) {}
    Tetromino(Matrix<int> shape, int type, int x, int y)
        : shape_(std::move(shape)), type_(type), x_(x), y_(y) {}

    const Matrix<int>& shape() const { return shape_; }
    int type() const { return type_; }
    int x() const { return x_; }
    int y() const { return y_; }

    std::vector<std::pair<int, int>> cells() const {
        std::vector<std::pair<int, int>> out;
        for (int r = 0; r < shape_.rows(); ++r)
            for (int c = 0; c < shape_.cols(); ++c)
                if (shape_[r][c] != 0) out.emplace_back(x_ + c, y_ + r);
        return out;
    }

    Tetromino movedBy(int dx, int dy) const {
        Tetromino t(*this);
        t.x_ += dx;
        t.y_ += dy;
        return t;
    }
    Tetromino rotatedCW() const { return Tetromino(shape_.rotatedCW(), type_, x_, y_); }

private:
    Matrix<int> shape_;
    int type_;
    int x_, y_;
};

class TetrominoFactory {
public:
    TetrominoFactory()
        : rng_(static_cast<unsigned>(
              std::random_device{}() ^
              static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()))) {}

    Tetromino next() {
        if (bag_.empty()) refillBag();
        int type = bag_.back();
        bag_.pop_back();
        return make(type);
    }

    // Piece of a given type (1=I 2=O 3=T 4=S 5=Z 6=J 7=L) at its spawn position.
    Tetromino make(int type) const {
        Matrix<int> s = shapeFor(type);
        int x = (COLS - s.cols()) / 2;
        return Tetromino(s, type, x, 0);
    }

private:
    void refillBag() {
        bag_ = {1, 2, 3, 4, 5, 6, 7};
        std::shuffle(bag_.begin(), bag_.end(), rng_);
    }

    static Matrix<int> build(std::initializer_list<std::initializer_list<int>> rows, int type) {
        int nr = static_cast<int>(rows.size());
        int nc = static_cast<int>(rows.begin()->size());
        Matrix<int> m(nr, nc, 0);
        int r = 0;
        for (const auto& row : rows) {
            int c = 0;
            for (int v : row) {
                m[r][c] = v ? type : 0;
                ++c;
            }
            ++r;
        }
        return m;
    }

    static Matrix<int> shapeFor(int type) {
        switch (type) {
            case 1: return build({{0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}}, 1);  // I
            case 2: return build({{1, 1}, {1, 1}}, 2);                                           // O
            case 3: return build({{0, 1, 0}, {1, 1, 1}, {0, 0, 0}}, 3);                          // T
            case 4: return build({{0, 1, 1}, {1, 1, 0}, {0, 0, 0}}, 4);                          // S
            case 5: return build({{1, 1, 0}, {0, 1, 1}, {0, 0, 0}}, 5);                          // Z
            case 6: return build({{1, 0, 0}, {1, 1, 1}, {0, 0, 0}}, 6);                          // J
            default: return build({{0, 0, 1}, {1, 1, 1}, {0, 0, 0}}, 7);                         // L
        }
    }

    std::mt19937 rng_;
    std::vector<int> bag_;
};
