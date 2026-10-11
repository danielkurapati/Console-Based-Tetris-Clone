#pragma once

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

template <typename T>
class Matrix {
public:
    class RowProxy {
    public:
        RowProxy(T* row, int cols) : row_(row), cols_(cols) {}
        T& operator[](int c) {
            if (c < 0 || c >= cols_) throw std::out_of_range("Matrix: column out of range");
            return row_[c];
        }
    private:
        T* row_;
        int cols_;
    };

    Matrix(int rows = 1, int cols = 1, T fillValue = T{})
        : rows_(rows), cols_(cols),
          data_(static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols), fillValue) {}

    int rows() const { return rows_; }
    int cols() const { return cols_; }

    bool inBounds(int r, int c) const {
        return r >= 0 && r < rows_ && c >= 0 && c < cols_;
    }

    RowProxy operator[](int r) {
        if (r < 0 || r >= rows_) throw std::out_of_range("Matrix: row out of range");
        return RowProxy(&data_[static_cast<std::size_t>(r) * cols_], cols_);
    }

    void fill(T value) { std::fill(data_.begin(), data_.end(), value); }

private:
    int rows_;
    int cols_;
    std::vector<T> data_;   // flat, row-major
};

