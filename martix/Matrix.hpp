//
// Created by taganyer on 2026/2/7.
//

#pragma once

#include "../log.hpp"
#include <fstream>
#include <memory>
#include <cassert>
#include <cstring>
#include <vector>
#include <random>

template<typename T>
class Matrix {
public:
    using Matrix_Array_1D = std::vector<Matrix>;

    using Matrix_Array_2D = std::vector<Matrix_Array_1D>;

    using Matrix_Array_3D = std::vector<Matrix_Array_2D>;

    Matrix() = default;

    explicit Matrix(std::istream &in) {
        size_t row = 0, col = 0;
        in >> row >> col;

        _row = row;
        _col = col;

        _data = std::make_unique<T[]>(row * col);

        for (size_t i = 0; i < row; ++i) {
            for (size_t j = 0; j < col; ++j) {
                in >> at(i, j);
            }
        }
    };

    Matrix(size_t rows, size_t cols, T min_val, T max_val) : Matrix(rows, cols) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<T> dist(min_val, max_val); // 均匀分布

        for (size_t i = 0; i < rows * cols; ++i) {
            _data[i] = dist(gen);
        }
    };

    Matrix(size_t rows, size_t cols) : _row(rows), _col(cols), _data(std::make_unique<T[]>(rows * cols)) {};

    Matrix(Matrix &&other) noexcept : _row(other._row), _col(other._col), _data(std::move(other._data)) {
        other._row = other._col = 0;
    };

    Matrix &operator=(Matrix &&other) noexcept {
        if (this == &other) return *this;
        _row = other._row;
        _col = other._col;
        _data = std::move(other._data);
        other._row = other._col = 0;
        return *this;
    }

    [[nodiscard]] Matrix copy() const {
        Matrix result(_row, _col);
        std::memcpy(result._data.get(), _data.get(), serialize_len() - head_len());
        return result;
    };

    static constexpr size_t head_len() { return sizeof(size_t) * 2; };

    [[nodiscard]] size_t serialize_len() const { return sizeof(T) * _row * _col + head_len(); };

    Matrix &operator+=(const Matrix &other) {
        assert(other._row == _row && other._col == _col);

        for (size_t i = 0; i < _row; ++i) {
            for (size_t j = 0; j < _col; ++j) {
                at(i, j) += other.at(i, j);
            }
        }

        return *this;
    };

    friend Matrix operator+(const Matrix &a, const Matrix &b) {
        assert(a._row == b._row && a._col == b._col);
        Matrix result(a._row, a._col);

        for (size_t i = 0; i < a._row; ++i) {
            for (size_t j = 0; j < b._col; ++j) {
                result.at(i, j) = a.at(i, j) + b.at(i, j);
            }
        }

        return result;
    };

    friend Matrix operator*(const Matrix &a, const Matrix &b) {
        assert(a._col == b._row);
        Matrix result(a._row, b._col);

        for (size_t i = 0; i < a._row; ++i) {
            for (size_t j = 0; j < b._col; ++j) {
                T t {};
                for (size_t k = 0; k < a._col; ++k) {
                    t += a.at(i, k) * b.at(k, j);
                }
                result.at(i, j) = t;
            }
        }

        return result;
    };

    [[nodiscard]] Matrix_Array_2D split(size_t n) const {
        assert(_row % n == 0 && _col % n == 0);
        Matrix_Array_2D result(n);

        for (auto &v: result) {
            v.reserve(n);
            for (size_t i = 0; i < n; ++i) {
                v.emplace_back(_row / n, _col / n);
            }
        }

        size_t x = 0, y = 0;
        for (auto &v: result) {
            for (auto &m: v) {
                for (int i = 0; i < m._row; ++i) {
                    for (int j = 0; j < m._col; ++j) {
                        m.at(i, j) = at(x + i, y + j);
                    }
                }
                y += m._col;
            }
            y = 0;
            x += v[0]._row;
        }

        return result;
    }

    friend Matrix merge(const Matrix_Array_2D &mat) {
        size_t rows = 0, cols = 0;
        for (int i = 0; i < mat.size(); ++i) {
            rows += mat[i][0]._row;
        }

        for (int i = 0; i < mat[0].size(); ++i) {
            cols += mat[0][i]._col;
        }

        Matrix result(rows, cols);
        size_t x = 0, y = 0;
        for (auto &v: mat) {
            for (auto &m: v) {
                for (int i = 0; i < m._row; ++i) {
                    for (int j = 0; j < m._col; ++j) {
                        result.at(x + i, y + j) = m.at(i, j);
                    }
                }
                y += m._col;
            }
            y = 0;
            x += v[0]._row;
        }

        return result;
    };

    T &at(size_t i, size_t j) {
        return *get(i, j);
    };

    [[nodiscard]] const T &at(size_t i, size_t j) const {
        return *get(i, j);
    };

    [[nodiscard]] size_t row() const { return _row; };

    [[nodiscard]] size_t col() const { return _col; };

    void show(std::ostream &out) const {
        out << _row << " " << _col << std::endl;
        for (size_t i = 0; i < _row; ++i) {
            for (size_t j = 0; j < _col; ++j) {
                out << at(i, j) << " ";
            }
            out << std::endl;
        }
        out << std::endl;
    };

    friend bool compare(const Matrix &a, const Matrix &b) {
        if (a._row != b._row || a._col != b._col) return false;
        return std::memcmp(a._data.get(), b._data.get(), a._row * a._col * sizeof(T)) == 0;
    };

    friend std::pair<Matrix_Array_2D, Matrix_Array_2D>
    SUMMA2D_scatter(const Matrix &a, const Matrix &b, int n) {
        auto as = a.split(n), bs = b.split(n);

        Matrix<int>::Matrix_Array_2D left(n), right(n);

        for (int l = 0; l < n; ++l) {
            for (int i = 0; i < n; ++i) {
                left[l].emplace_back(std::move(as[i][l]));
            }
            for (int j = 0; j < n; ++j) {
                right[l].emplace_back(std::move(bs[l][j]));
            }
        }

        return { std::move(left), std::move(right) };
    }

    friend Matrix_Array_2D SUMMA2D_multiply(const Matrix_Array_1D &m1,
                                            const Matrix_Array_1D &m2) {
        Matrix_Array_2D result(m1.size());

        for (size_t i = 0; i < m1.size(); ++i) {
            result[i].reserve(m2.size());
            for (size_t j = 0; j < m2.size(); ++j) {
                result[i].emplace_back(m1[i] * m2[j]);
            }
        }

        return result;
    };

    friend Matrix_Array_2D SUMMA2D_reduce(const Matrix_Array_3D &mats) {
        size_t m = mats[0].size(), n = mats[0][0].size();
        Matrix_Array_2D ret(m);

        for (size_t i = 0; i < m; ++i) {
            for (size_t j = 0; j < n; ++j) {
                ret[i].emplace_back(mats[0][i][j].copy());
            }
        }

        for (size_t k = 1; k < mats.size(); ++k) {
            for (size_t i = 0; i < m; ++i) {
                for (size_t j = 0; j < n; ++j) {
                    ret[i][j] += mats[k][i][j];
                }
            }
        }

        return ret;
    };

    friend uint64_t serialize_len1D(const Matrix_Array_1D &mat) {
        uint64_t size = 0;
        for (auto& v: mat) {
            size += v.serialize_len();
        }
        return size;
    };

    friend uint64_t serialize_len2D(const Matrix_Array_2D &mat) {
        uint64_t size = 0;
        for (auto& v: mat) {
            size += serialize_len1D(v);
        }
        return size;
    };

    friend uint64_t serialize_len3D(const Matrix_Array_3D &mat) {
        uint64_t size = 0;
        for (auto& v: mat) {
            size += serialize_len2D(v);
        }
        return size;
    };

private:
    [[nodiscard]] T *get(size_t i, size_t j) const {
        return _data.get() + i * _col + j;
    };

    [[nodiscard]] char *get_raw(size_t pos) const {
        return reinterpret_cast<char *>(_data.get()) + pos;
    };

    size_t _row = 0, _col = 0;

    std::unique_ptr<T[]> _data;
};

template<typename T>
bool generate_random_matrix(const std::string &file_path,
                            size_t m,
                            size_t n,
                            T min_val,
                            T max_val) {
    try {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<T> dist(min_val, max_val); // 均匀分布

        std::ofstream out_file(file_path);
        if (!out_file.is_open()) {
            ERROR("错误：无法打开文件 %s", file_path.data());
            return false;
        }

        out_file << m << " " << n << std::endl;

        for (size_t i = 0; i < m; ++i) {
            for (size_t j = 0; j < n; ++j) {
                out_file << dist(gen);
                if (j != n - 1) {
                    out_file << " ";
                }
            }
            out_file << std::endl;
        }

        out_file.close();
        DEBUG("成功生成矩阵文件：%s", file_path.data());
        return true;
    } catch (const std::exception &e) {
        ERROR("生成矩阵失败：%s", e.what());
        return false;
    }
}
