//
// Created by taganyer on 2026/2/10.
//

#pragma once

#include <mutex>
#include <condition_variable>
#include "Matrix.hpp"
#include "../frame/ProcessingRules.hpp"

using MARTIX_TYPE = int;

using MARTIX = Matrix<MARTIX_TYPE>;

using MartixPtr = std::unique_ptr<MARTIX>;

using Martix1DPtr = std::unique_ptr<MARTIX::Matrix_Array_1D>;

using Martix2DPtr = std::unique_ptr<MARTIX::Matrix_Array_2D>;

using Martix3DPtr = std::unique_ptr<MARTIX::Matrix_Array_3D>;

class ReduceMartixRules;

class ScatterMartixRules : public Rules {
public:
    ScatterMartixRules(RulesID id, MARTIX m1, MARTIX m2, ReduceMartixRules *reduce) :
        _id(id), _m1(std::make_unique<MARTIX>(std::move(m1))),
        _m2(std::make_unique<MARTIX>(std::move(m2))), _reduce(reduce) {};

    RulesID guid() const override;

    void store(Processor &processor, const tdcf::DataPtr &data) override;

    void acquire(Processor &processor, tdcf::ProcessorEventMark mark) override;

    void reduce(Processor &processor, tdcf::ProcessorEventMark mark,
                tdcf::DataSet target) override;

    void scatter(Processor &processor, tdcf::ProcessorEventMark mark,
                 uint32_t scatter_size, tdcf::DataSet dataset) override;

    bool get_events(Processor &processor, tdcf::Processor::EventQueue &queue) override;

    void finish_callback() override;

private:
    using EventPtr = std::unique_ptr<tdcf::ProcessorEventMark>;

    struct RootData {
        EventPtr ac;
        EventPtr sc;
        uint32_t scatter_size = 0;
    };

    RulesID _id;

    std::mutex _mutex;

    MartixPtr _m1, _m2;

    Martix2DPtr _scatter_v1, _scatter_v2;

    RootData _root;

    ReduceMartixRules *_reduce;
};

class ReduceMartixRules : public Rules {
public:
    using Fun = std::function<void(MARTIX)>;

    ReduceMartixRules(RulesID id, Fun fun) : _id(id), _fun(std::move(fun)) {};

    RulesID guid() const override;

    void store(Processor &processor, const tdcf::DataPtr &data) override;

    void acquire(Processor &processor, tdcf::ProcessorEventMark mark) override;

    void reduce(Processor &processor, tdcf::ProcessorEventMark mark,
                tdcf::DataSet target) override;

    void scatter(Processor &processor, tdcf::ProcessorEventMark mark,
                 uint32_t scatter_size, tdcf::DataSet dataset) override;

    bool get_events(Processor &processor, tdcf::Processor::EventQueue &queue) override;

    void finish_callback() override;

private:
    void set_scatter_data(Martix2DPtr v1, Martix2DPtr v2);

    using EventPtr = std::unique_ptr<tdcf::ProcessorEventMark>;

    struct Data {
        EventPtr ac;
        EventPtr re;
        uint32_t reduce_size = 0;
    };

    Data &get_data(Processor &processor);

    RulesID _id;

    std::mutex _mutex;

    std::condition_variable _cond;

    Martix2DPtr _scatter_v1, _scatter_v2;

    MARTIX::Matrix_Array_3D _subs;

    MARTIX _ans;

    std::unordered_map<tdcf::Identity::Uid, Data> _m;

    Fun _fun;

    friend class ScatterMartixRules;
};
