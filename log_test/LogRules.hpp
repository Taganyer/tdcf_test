//
// Created by taganyer on 2026/2/10.
//

#pragma once

#include <mutex>

#include "../frame/ProcessingRules.hpp"

class LogRules : public Rules {
public:
    explicit LogRules(RulesID id) : _id(id) {};

    RulesID guid() const override {
        return _id;
    };

    void store(Processor &processor, const tdcf::DataPtr &data) override;

    void acquire(Processor &processor, tdcf::ProcessorEventMark mark) override;

    void reduce(Processor &processor, tdcf::ProcessorEventMark mark,
                tdcf::DataSet target) override;

    void scatter(Processor &processor, tdcf::ProcessorEventMark mark,
                 uint32_t scatter_size, tdcf::DataSet dataset) override;

    bool get_events(Processor &processor, tdcf::Processor::EventQueue &queue) override;

    void finish_callback() override;

private:
    tdcf::Processor::EventQueue &get_queue(Processor &processor);

    RulesID _id;

    std::mutex _mutex;

    std::unordered_map<tdcf::Identity::Uid, tdcf::Processor::EventQueue> _queue;
};
