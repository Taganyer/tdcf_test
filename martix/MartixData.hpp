//
// Created by taganyer on 2026/2/9.
//

#pragma once

#include <tdcf/frame/Data.hpp>

#include "Matrix.hpp"

template<typename T>
class MatrixData : public tdcf::Data {
public:
    explicit MatrixData(Matrix<T> matrix) : data(std::move(matrix)) {};

    tdcf::SerializableType derived_type() const override {
        return 1;
    };

    uint64_t serialize_size() const override {
        return data.serialize_len();
    };

    Matrix<T> data;
};

template<typename T>
class MatrixData1D : public tdcf::Data {
public:
    explicit MatrixData1D(typename Matrix<T>::MatrixData1D matrix_data_1d) :
        data(std::move(matrix_data_1d)) {
        for (auto &m: data) {
            _size += m.serialize_len();
        }
    };

    tdcf::SerializableType derived_type() const override {
        return 2;
    };

    uint64_t serialize_size() const override {
        return _size;
    };


    typename Matrix<T>::MatrixData1D data;

private:
    uint64_t _size = 0;
};

template<typename T>
class MatrixData2D : public tdcf::Data {
public:
    explicit MatrixData2D(typename Matrix<T>::MatrixData2D matrix_data_2d) :
        data(std::move(matrix_data_2d)) {
        for (auto &arr: data) {
            for (auto &m: arr) {
                _size += m.serialize_len();
            }
        }
    };

    tdcf::SerializableType derived_type() const override {
        return 3;
    };

    uint64_t serialize_size() const override {
        return _size;
    };

    typename Matrix<T>::MatrixData2D data;

private:
    uint64_t _size = 0;
};

template<typename T>
class MatrixData3D : public tdcf::Data {
public:
    explicit MatrixData3D(typename Matrix<T>::MatrixData3D matrix_data_3d) :
        data(std::move(matrix_data_3d)) {
        for (auto &arr1: data) {
            for (auto &arr2: arr1) {
                for (auto &m: arr2) {
                    _size += m.serialize_len();
                }
            }
        }
    };

    tdcf::SerializableType derived_type() const override {
        return 4;
    };

    uint64_t serialize_size() const override {
        return _size;
    };

    typename Matrix<T>::MatrixData3D data;

private:
    uint64_t _size = 0;
};
