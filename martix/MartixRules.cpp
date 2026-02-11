//
// Created by taganyer on 2026/2/10.
//

#include "../martix/MartixRules.hpp"

RulesID MartixRules::guid() const {
    return 0;
}

void MartixRules::store(Processor &processor, const tdcf::DataPtr &data) {}

void MartixRules::acquire(Processor &processor, tdcf::ProcessorEventMark mark) {}

void MartixRules::reduce(Processor &processor, tdcf::ProcessorEventMark mark,
                         tdcf::DataSet target) {}

void MartixRules::scatter(Processor &processor, tdcf::ProcessorEventMark mark,
                          uint32_t scatter_size, tdcf::DataSet dataset) {}

bool MartixRules::get_events(Processor &processor, tdcf::Processor::EventQueue &queue) {
    return true;
}
