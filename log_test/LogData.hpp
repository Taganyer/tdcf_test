//
// Created by taganyer on 2026/2/10.
//

#pragma once

#include <string>
#include <tdcf/frame/Data.hpp>

class LogData : public tdcf::Data {
public:
    LogData(std::string data) : _data(std::move(data)) {};

    tdcf::SerializableType derived_type() const override {
        return 2;
    };

    uint64_t serialize_size() const override {
        return 1000 + _data.size();
    };

    const char *data() const {
        return _data.c_str();
    };

private:
    std::string _data;

};
