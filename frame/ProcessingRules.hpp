//
// Created by taganyer on 2026/2/9.
//

#pragma once

#include <functional>

#include "Processor.hpp"

class Rules : public tdcf::ProcessingRules {
public:
    uint64_t serialize_size() const override {
        return sizeof(ProcessingRules);
    };

    Result serialize(void *buffer, uint64_t buffer_size, uint64_t pos) const override {
        return { 0, true };
    };

    Result deserialize(const void *buffer, uint64_t buffer_size, uint64_t pos) override {
        return { 0, true };
    };

    virtual RulesID guid() const = 0;

    virtual void store(Processor &processor, const tdcf::DataPtr &data) = 0;

    virtual void acquire(Processor &processor, tdcf::ProcessorEventMark mark) = 0;

    virtual void reduce(Processor &processor, tdcf::ProcessorEventMark mark,
                        tdcf::DataSet target) = 0;

    virtual void scatter(Processor& processor, tdcf::ProcessorEventMark mark,
                         uint32_t scatter_size, tdcf::DataSet dataset) = 0;

    virtual bool get_events(Processor& processor, tdcf::Processor::EventQueue &queue) = 0;

    using Callback = std::function<void()>;

    virtual void set_callback(Callback callback) {
        _callback = std::move(callback);
    };

protected:
    Callback _callback;
};
