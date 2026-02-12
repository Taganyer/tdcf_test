//
// Created by taganyer on 2026/2/9.
//

#pragma once

#include <tdcf/frame/Data.hpp>


class MatrixData : public tdcf::Data {
public:
    explicit MatrixData(uint64_t size) : _size(size) {};

    tdcf::SerializableType derived_type() const override {
        return 1;
    };

    uint64_t serialize_size() const override {
        return _size;
    };

private:
    uint64_t _size;
};
