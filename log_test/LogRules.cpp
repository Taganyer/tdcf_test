//
// Created by taganyer on 2026/2/10.
//

#include "LogRules.hpp"
#include "LogData.hpp"
#include "../log.hpp"


void LogRules::store(Processor &processor, const tdcf::DataPtr &data) {
    auto &p = *processor.get_identity();
    auto &d = dynamic_cast<LogData &>(*data);
    INFO("processor[%s(%u)-%u] rule[%lu] store data:{%s}",
         p.mod_str(), p.guid(), p.weight(), _id, d.data());
}

void LogRules::acquire(Processor &processor, tdcf::ProcessorEventMark mark) {
    auto &p = *processor.get_identity();
    INFO("processor[%s(%u)-%u] rule[%lu] acquire data",
         p.mod_str(), p.guid(), p.weight(), _id);

    auto text = "[acquire(" + std::to_string(_id) + ")]";

    auto &q = get_queue(processor);
    q.emplace(tdcf::ProcessorEvent {
        tdcf::ProcessorEvent::Acquire, mark,
        tdcf::DataSet { std::make_shared<LogData>(std::move(text)) }
    });
}

void LogRules::reduce(Processor &processor, tdcf::ProcessorEventMark mark,
                      tdcf::DataSet target) {
    auto &p = *processor.get_identity();
    INFO("processor[%s(%u)-%u] rule[%lu] reduce %lu datas",
         p.mod_str(), p.guid(), p.weight(), _id, target.size());

    auto text("[(" + std::to_string(p.guid()) + ")reduce{");
    for (auto &ptr: target) {
        if (ptr->derived_type() != 2) continue;
        auto &d = dynamic_cast<LogData &>(*ptr);
        text.append(d.data());
        text.push_back(',');
    }
    text.append("}]");

    auto &q = get_queue(processor);
    q.emplace(tdcf::ProcessorEvent {
        tdcf::ProcessorEvent::Reduce, mark,
        tdcf::DataSet { std::make_shared<LogData>(std::move(text)) }
    });
}

void LogRules::scatter(Processor &processor, tdcf::ProcessorEventMark mark,
                       uint32_t scatter_size, tdcf::DataSet dataset) {
    auto &p = *processor.get_identity();
    INFO("processor[%s(%u)-%u] rule[%lu] scatter to %u datas",
         p.mod_str(), p.guid(), p.weight(), _id, scatter_size);

    auto &d = dynamic_cast<LogData &>(*dataset[0]);

    tdcf::DataSet target;

    for (uint32_t i = 0; i < scatter_size; ++i) {
        auto text = std::string(d.data()) + "[scatter[" + std::to_string(i) + "]]";
        target.emplace_back(std::make_shared<LogData>(std::move(text)));
    }

    auto &q = get_queue(processor);
    q.emplace(tdcf::ProcessorEvent {
        tdcf::ProcessorEvent::Scatter, mark, std::move(target)
    });
}

bool LogRules::get_events(Processor &processor, tdcf::Processor::EventQueue &queue) {
    auto &q = get_queue(processor);
    while (!q.empty()) {
        auto &e = q.front();
        queue.emplace(std::move(e));
        q.pop();
    }
    return true;
}

void LogRules::finish_callback() {
    if (_callback) {
        _callback();
        INFO(__PRETTY_FUNCTION__);
    }
}

tdcf::Processor::EventQueue &LogRules::get_queue(Processor &processor) {
    std::lock_guard l(_mutex);
    return _queue[processor.get_identity()->guid()];
}
