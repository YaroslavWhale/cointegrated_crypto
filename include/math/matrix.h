#pragma once
#include <array>
#include <cmath>
#include <stdexcept>

template<typename T, size_t Rows, size_t Cols>
class Matrix {
public:
    std::array<std::array<T, Cols>, Rows> data;

    Matrix() { zero(); }
    void zero() {
        for (auto& row : data)
            row.fill(T(0));
    }
    void identity() {
        zero();
        for (size_t i = 0; i < std::min(Rows, Cols); ++i)
            data[i][i] = T(1);
    }

    Matrix operator+(const Matrix& other) const {
        Matrix res;
        for (size_t i = 0; i < Rows; ++i)
            for (size_t j = 0; j < Cols; ++j)
                res.data[i][j] = data[i][j] + other.data[i][j];
        return res;
    }

    Matrix operator-(const Matrix& other) const {
        Matrix res;
        for (size_t i = 0; i < Rows; ++i)
            for (size_t j = 0; j < Cols; ++j)
                res.data[i][j] = data[i][j] - other.data[i][j];
        return res;
    }

    template<size_t OtherCols>
    Matrix<T, Rows, OtherCols> operator*(const Matrix<T, Cols, OtherCols>& other) const {
        Matrix<T, Rows, OtherCols> res;
        res.zero();
        for (size_t i = 0; i < Rows; ++i)
            for (size_t k = 0; k < Cols; ++k)
                for (size_t j = 0; j < OtherCols; ++j)
                    res.data[i][j] += data[i][k] * other.data[k][j];
        return res;
    }

    Matrix<T, Cols, Rows> transpose() const {
        Matrix<T, Cols, Rows> res;
        for (size_t i = 0; i < Rows; ++i)
            for (size_t j = 0; j < Cols; ++j)
                res.data[j][i] = data[i][j];
        return res;
    }

    T trace() const {
        static_assert(Rows == Cols, "trace only for square matrix");
        T tr = 0;
        for (size_t i = 0; i < Rows; ++i)
            tr += data[i][i];
        return tr;
    }

    // Скалярное умножение
    Matrix operator*(T scalar) const {
        Matrix res;
        for (size_t i = 0; i < Rows; ++i)
            for (size_t j = 0; j < Cols; ++j)
                res.data[i][j] = data[i][j] * scalar;
        return res;
    }

    // Обращение скаляра (1x1)
    T invScalar() const {
        static_assert(Rows == 1 && Cols == 1, "invScalar only for 1x1 matrix");
        T val = data[0][0];
        if (val < 1e-12) val = 1e-12;
        return T(1) / val;
    }

    // Для квадратных матриц 2x2 и 3x3 — обращение (нужно только для демонстрации, в фильтре не используется)
    // Но понадобится только инверсия скаляра S, так что остальное не обязательно.
};

template<typename T, size_t Dim>
using Vector = Matrix<T, Dim, 1>;

// Вспомогательный вывод (опционально)
#include <iostream>
template<typename T, size_t R, size_t C>
std::ostream& operator<<(std::ostream& os, const Matrix<T, R, C>& m) {
    for (size_t i = 0; i < R; ++i) {
        os << "[ ";
        for (size_t j = 0; j < C; ++j)
            os << m.data[i][j] << " ";
        os << "]\n";
    }
    return os;
}
