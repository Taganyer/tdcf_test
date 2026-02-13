//
// Created by taganyer on 2026/2/10.
//

#include "MartixRules.hpp"
#include "MartixData.hpp"

RulesID ScatterMartixRules::guid() const {
    return _id;
}

void ScatterMartixRules::store(Processor &processor, const tdcf::DataPtr &data) {
    assert(processor.get_identity()->mod() != Identity::NODE_ROOT);
}

void ScatterMartixRules::acquire(Processor &processor, tdcf::ProcessorEventMark mark) {
    assert(processor.get_identity()->mod() == Identity::ROOT);
    _root.ac = std::make_unique<tdcf::ProcessorEventMark>(mark);
}

void ScatterMartixRules::reduce(Processor &processor, tdcf::ProcessorEventMark mark,
                                tdcf::DataSet target) {
    assert(false);
}

void ScatterMartixRules::scatter(Processor &processor, tdcf::ProcessorEventMark mark,
                                 uint32_t scatter_size, tdcf::DataSet dataset) {
    assert(processor.get_identity()->mod() == Identity::ROOT);
    _root.sc = std::make_unique<tdcf::ProcessorEventMark>(mark);
    _root.scatter_size = scatter_size;
}

bool ScatterMartixRules::get_events(Processor &processor, tdcf::Processor::EventQueue &queue) {
    assert(processor.get_identity()->mod() != Identity::NODE_ROOT);
    if (processor.get_identity()->mod() == Identity::ROOT) {
        if (_root.ac) {
            auto ptr = std::make_shared<MatrixData>(_m1->serialize_len() + _m2->serialize_len());
            queue.emplace(tdcf::ProcessorEvent {
                tdcf::ProcessorEvent::Acquire, *_root.ac,
                tdcf::DataSet { std::move(ptr) }
            });
            _root.ac.reset();
        } else if (_root.sc) {
            auto [fst, snd] = SUMMA2D_scatter(
                *_m1, *_m2, processor.get_identity()->weight());
            _m1.reset();
            _m2.reset();
            _scatter_v1 = std::make_unique<MARTIX::Matrix_Array_2D>(std::move(fst));
            _scatter_v2 = std::make_unique<MARTIX::Matrix_Array_2D>(std::move(snd));

            auto len = serialize_len2D(*_scatter_v1) + serialize_len2D(*_scatter_v2);
            tdcf::DataSet set;
            set.reserve(_root.scatter_size);
            for (uint32_t i = 0; i < _root.scatter_size; ++i) {
                set.emplace_back(std::make_shared<MatrixData>(len / _root.scatter_size));
            }
            queue.emplace(tdcf::ProcessorEvent {
                tdcf::ProcessorEvent::Scatter, *_root.sc, std::move(set)
            });

            _root.sc.reset();
        }
    }
    return true;
}

void ScatterMartixRules::finish_callback() {
    _reduce->set_scatter_data(std::move(_scatter_v1), std::move(_scatter_v2));
    if (_callback) _callback();
}

RulesID ReduceMartixRules::guid() const {
    return _id;
}

void ReduceMartixRules::store(Processor &processor, const tdcf::DataPtr &data) {
    assert(processor.get_identity()->mod() == Identity::ROOT);
    std::lock_guard l(_mutex);
    assert(_subs.size() == 1);
    _ans = merge(_subs.front());
    _subs.clear();
}

void ReduceMartixRules::acquire(Processor &processor, tdcf::ProcessorEventMark mark) {
    assert(processor.get_identity()->mod() != Identity::NODE_ROOT);
    auto &d = get_data(processor);
    d.ac = std::make_unique<tdcf::ProcessorEventMark>(mark);
}

void ReduceMartixRules::reduce(Processor &processor, tdcf::ProcessorEventMark mark,
                               tdcf::DataSet target) {
    assert(processor.get_identity()->mod() != Identity::NODE_ROOT);
    auto &d = get_data(processor);
    d.re = std::make_unique<tdcf::ProcessorEventMark>(mark);
    d.reduce_size = target.size();
    assert(d.reduce_size);
}

void ReduceMartixRules::scatter(Processor &processor, tdcf::ProcessorEventMark mark,
                                uint32_t scatter_size, tdcf::DataSet dataset) {
    assert(false);
}

bool ReduceMartixRules::get_events(Processor &processor, tdcf::Processor::EventQueue &queue) {
    assert(processor.get_identity()->mod() != Identity::NODE_ROOT);
    auto &[ac, re, reduce_size] = get_data(processor);
    if (ac) {
        MARTIX::Matrix_Array_1D v1, v2;
        {
            std::lock_guard l(_mutex);
            if (!_scatter_v1) return true;
            v1 = std::move(_scatter_v1->back());
            v2 = std::move(_scatter_v2->back());
            _scatter_v1->pop_back();
            _scatter_v2->pop_back();
        }
        auto ans = SUMMA2D_multiply(v1, v2);

        auto ptr = std::make_shared<MatrixData>(serialize_len2D(ans));
        queue.emplace(tdcf::ProcessorEvent {
            tdcf::ProcessorEvent::Acquire, *ac,
            tdcf::DataSet { std::move(ptr) }
        });
        ac.reset();

        std::lock_guard l(_mutex);
        _subs.emplace_back(std::move(ans));
    } else if (re) {
        MARTIX::Matrix_Array_3D v;
        {
            std::lock_guard l(_mutex);
            assert(reduce_size <= _subs.size());
            for (uint32_t i = 0; i < reduce_size; ++i) {
                v.emplace_back(std::move(_subs.back()));
                _subs.pop_back();
            }
        }
        auto ans = v.size() != 1 ? SUMMA2D_reduce(v) : std::move(v.front());

        auto ptr = std::make_shared<MatrixData>(serialize_len2D(ans));
        queue.emplace(tdcf::ProcessorEvent {
            tdcf::ProcessorEvent::Reduce, *re,
            tdcf::DataSet { std::move(ptr) }
        });
        re.reset();

        std::lock_guard l(_mutex);
        _subs.emplace_back(std::move(ans));
    }

    return true;
}

void ReduceMartixRules::finish_callback() {
    _fun(std::move(_ans));
    if (_callback) _callback();
}

void ReduceMartixRules::set_scatter_data(Martix2DPtr v1, Martix2DPtr v2) {
    std::lock_guard l(_mutex);
    _scatter_v1 = std::move(v1);
    _scatter_v2 = std::move(v2);
}

ReduceMartixRules::Data &ReduceMartixRules::get_data(Processor &processor) {
    std::lock_guard l(_mutex);
    return _m[processor.get_identity()->guid()];
}
