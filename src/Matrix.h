
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
        RowProxy(T* row, int cols)
            : row_(row), cols_(cols) {}

        T& operator[](int c) {
            if (c < 0 || c >= cols_) {
                throw std::out_of_range(
                    "Matrix: column out of range"
                );
            }

            return row_[c];
        }

    private:
        T* row_;
        int cols_;
    };

    class ConstRowProxy {
    public:
        ConstRowProxy(const T* row, int cols)
            : row_(row), cols_(cols) {}

        const T& operator[](int c) const {
            if (c < 0 || c >= cols_) {
                throw std::out_of_range(
                    "Matrix: column out of range"
                );
            }

            return row_[c];
        }

    private:
        const T* row_;
        int cols_;
    };

    Matrix(int rows = 1, int cols = 1, T fillValue = T{})
        : rows_(rows),
          cols_(cols),
          data_(checkedSize(rows, cols), fillValue) {}

    int rows() const {
        return rows_;
    }

    int cols() const {
        return cols_;
    }

    bool empty() const {
        return rows_ == 0 || cols_ == 0;
    }

    bool inBounds(int r, int c) const {
        return r >= 0 && r < rows_
            && c >= 0 && c < cols_;
    }

    T& at(int r, int c) {
        if (!inBounds(r, c)) {
            throw std::out_of_range(
                "Matrix: index out of range"
            );
        }

        return data_[
            static_cast<std::size_t>(r) * cols_ + c
        ];
    }

    const T& at(int r, int c) const {
        if (!inBounds(r, c)) {
            throw std::out_of_range(
                "Matrix: index out of range"
            );
        }

        return data_[
            static_cast<std::size_t>(r) * cols_ + c
        ];
    }

    RowProxy operator[](int r) {
        if (r < 0 || r >= rows_) {
            throw std::out_of_range(
                "Matrix: row out of range"
            );
        }

        return RowProxy(
            data_.data() +
                static_cast<std::size_t>(r) * cols_,
            cols_
        );
    }

    ConstRowProxy operator[](int r) const {
        if (r < 0 || r >= rows_) {
            throw std::out_of_range(
                "Matrix: row out of range"
            );
        }

        return ConstRowProxy(
            data_.data() +
                static_cast<std::size_t>(r) * cols_,
            cols_
        );
    }

    void fill(const T& value) {
        std::fill(data_.begin(), data_.end(), value);
    }

    Matrix transposed() const {
        Matrix result(cols_, rows_);

        for (int r = 0; r < rows_; ++r) {
            for (int c = 0; c < cols_; ++c) {
                result[c][r] = (*this)[r][c];
            }
        }

        return result;
    }

    Matrix rotatedCW() const {
        Matrix result(cols_, rows_);

        for (int r = 0; r < rows_; ++r) {
            for (int c = 0; c < cols_; ++c) {
                result[c][rows_ - 1 - r] =
                    (*this)[r][c];
            }
        }

        return result;
    }

    Matrix rotatedCCW() const {
        Matrix result(cols_, rows_);

        for (int r = 0; r < rows_; ++r) {
            for (int c = 0; c < cols_; ++c) {
                result[cols_ - 1 - c][r] =
                    (*this)[r][c];
            }
        }

        return result;
    }

private:
    static std::size_t checkedSize(int rows, int cols) {
        if (rows < 0 || cols < 0) {
            throw std::invalid_argument(
                "Matrix: dimensions cannot be negative"
            );
        }

        return static_cast<std::size_t>(rows)
            * static_cast<std::size_t>(cols);
    }

    int rows_;
    int cols_;
    std::vector<T> data_;
};
