//
// Created by taganyer on 2026/2/9.
//

#include "Processor.hpp"
#include "ProcessingRules.hpp"


void Processor::store(const tdcf::ProcessingRulesPtr &rule_ptr,
                      const tdcf::DataPtr &data_ptr) {
    auto &rule = get_rule(rule_ptr);
    rule.store(*this, data_ptr);
}

void Processor::acquire(tdcf::ProcessorEventMark mark,
                        const tdcf::ProcessingRulesPtr &rule_ptr) {
    auto &rule = get_rule(rule_ptr);
    rule.acquire(*this, mark);
}

void Processor::reduce(tdcf::ProcessorEventMark mark,
                       const tdcf::ProcessingRulesPtr &rule_ptr,
                       tdcf::DataSet target) {
    auto &rule = get_rule(rule_ptr);
    rule.reduce(*this, mark, std::move(target));
}

void Processor::scatter(tdcf::ProcessorEventMark mark,
                        const tdcf::ProcessingRulesPtr &rule_ptr,
                        uint32_t scatter_size,
                        tdcf::DataSet dataset) {
    auto &rule = get_rule(rule_ptr);
    rule.scatter(*this, mark, scatter_size, std::move(dataset));
}

tdcf::OperationFlag Processor::get_events(EventQueue &queue) {
    auto iter = _rules.begin();
    auto size = queue.size();
    while (iter != _rules.end()) {
        auto &[id, rule] = *iter;
        if (!rule->get_events(*this, queue)) {
            iter = _rules.erase(iter);
        } else {
            ++iter;
        }
    }
    return size != queue.size() ? tdcf::OperationFlag::Success : tdcf::OperationFlag::FurtherWaiting;
}

Rules &Processor::get_rule(const tdcf::ProcessingRulesPtr &rule_ptr) {
    auto ptr = std::dynamic_pointer_cast<Rules>(rule_ptr);
    auto &rule = *ptr;
    _rules.try_emplace(rule.guid(), std::move(ptr));
    return rule;
};
