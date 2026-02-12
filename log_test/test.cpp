//
// Created by taganyer on 2026/2/10.
//
#include "test.hpp"
#include "LogRules.hpp"
#include "../manage/Manager.hpp"

static ManagerPtr create(uint64_t &id,
                         tdcf::StageNum topo_type, int64_t intra, int64_t inter,
                         int broadcast, int scatter, int reduce,
                         int all_reduce, int reduce_scatter,
                         int nodes) {
    auto ptr = std::make_unique<Manager>(topo_type, intra, inter);

    while (broadcast) {
        ptr->add_operation(tdcf::OperationType::Broadcast, std::make_shared<LogRules>(++id));
        --broadcast;
    }

    while (scatter) {
        ptr->add_operation(tdcf::OperationType::Scatter, std::make_shared<LogRules>(++id));
        --scatter;
    }

    while (reduce) {
        ptr->add_operation(tdcf::OperationType::Reduce, std::make_shared<LogRules>(++id));
        --reduce;
    }

    while (all_reduce) {
        ptr->add_operation(tdcf::OperationType::AllReduce, std::make_shared<LogRules>(++id));
        --all_reduce;
    }

    while (reduce_scatter) {
        ptr->add_operation(tdcf::OperationType::ReduceScatter, std::make_shared<LogRules>(++id));
        --reduce_scatter;
    }

    for (int i = 0; i < nodes; ++i) {
        auto sub = std::make_unique<Manager>(topo_type, intra, inter);
        ptr->add_sub_cluster(std::move(sub));
    }

    return ptr;
}


void log_test() {
    uint64_t rule_id = 0;
    auto star = create(rule_id, tdcf::ClusterType::star, 1LL << 33, 1LL << 32,
                       1, 1, 1, 1, 1, 3);
    auto ring = create(rule_id, tdcf::ClusterType::ring, 1LL << 33, 1LL << 32,
                       1, 1, 1, 1, 1, 3);
    auto dbt = create(rule_id, tdcf::ClusterType::dbt, 1LL << 33, 1LL << 32,
                      1, 1, 1, 1, 1, 3);

    star->add_sub_cluster(std::move(ring));
    star->add_sub_cluster(std::move(dbt));

    star->root_init();
    star->start();

    star.reset();
    Communicator::clear_comm();
};
