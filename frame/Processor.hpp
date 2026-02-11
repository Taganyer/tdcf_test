//
// Created by taganyer on 2026/2/9.
//

#pragma once

#include <unordered_map>
#include <tdcf/frame/Processor.hpp>
#include "Identity.hpp"

class Rules;

using RulePtr = std::shared_ptr<Rules>;

using RulesID = uint64_t;

class Processor : public tdcf::Processor {
public:
    explicit Processor(std::shared_ptr<Identity> id) : _id(std::move(id)) {};

    void store(const tdcf::ProcessingRulesPtr &rule_ptr, const tdcf::DataPtr &data_ptr) override;

    void acquire(tdcf::ProcessorEventMark mark, const tdcf::ProcessingRulesPtr &rule_ptr) override;

    void reduce(tdcf::ProcessorEventMark mark, const tdcf::ProcessingRulesPtr &rule_ptr,
                tdcf::DataSet target) override;

    void scatter(tdcf::ProcessorEventMark mark, const tdcf::ProcessingRulesPtr &rule_ptr,
                 uint32_t scatter_size, tdcf::DataSet dataset) override;

    tdcf::OperationFlag get_events(EventQueue &queue) override;

    const std::shared_ptr<Identity> &get_identity() { return _id; };

private:
    Rules &get_rule(const tdcf::ProcessingRulesPtr &rule_ptr);

    std::shared_ptr<Identity> _id;

    std::unordered_map<RulesID, RulePtr> _rules;
};
