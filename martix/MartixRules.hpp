//
// Created by taganyer on 2026/2/10.
//

#pragma once

#include <mutex>
#include "../frame/ProcessingRules.hpp"

class MartixRules : public Rules {
public:
    RulesID guid() const override;

    void store(Processor &processor, const tdcf::DataPtr &data) override;

    void acquire(Processor &processor, tdcf::ProcessorEventMark mark) override;

    void reduce(Processor &processor, tdcf::ProcessorEventMark mark,
                tdcf::DataSet target) override;

    void scatter(Processor &processor, tdcf::ProcessorEventMark mark,
                 uint32_t scatter_size, tdcf::DataSet dataset) override;

    bool get_events(Processor &processor, tdcf::Processor::EventQueue &queue) override;

private:
    std::mutex _mutex;

    // std::unordered_map<tdcf::Identity::Uid, >;
};
