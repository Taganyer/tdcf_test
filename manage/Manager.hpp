//
// Created by taganyer on 2026/2/10.
//

#pragma once

#include "../frame/Processor.hpp"
#include "../frame/Communicator.hpp"
#include <atomic>
#include <thread>
#include <tdcf/cluster/Cluster.hpp>


class Manager;

using ManagerPtr = std::unique_ptr<Manager>;

class Manager {
public:
    Manager(tdcf::StageNum topo_type, int64_t intra, int64_t inter);

    ~Manager();

    void add_operation(tdcf::OperationType type, RulePtr rule);

    void add_sub_cluster(ManagerPtr sub);

    void root_init();

    void start();

private:
    void create_tasks(tdcf::Cluster &root);

    std::shared_ptr<tdcf::Cluster> create_cluster();

    tdcf::Node create_node();

    void root();

    void node_root();

    void pure_node();

    void init(uint32_t &id, std::vector<std::vector<int64_t> > &comm_matrix,
              std::vector<tdcf::IdentityPtr> &targets);

    std::vector<std::pair<tdcf::OperationType, RulePtr> > _operations;

    tdcf::StageNum _type = 0;

    std::atomic<bool> _running;

    uint32_t _total = 1;

    int64_t _intra, _inter;

    std::atomic<uint32_t> _tasks_size = 0;

    std::shared_ptr<Identity> _id, _root;

    std::shared_ptr<Processor> _processor;

    std::thread _thread;

    std::vector<ManagerPtr> _subs;
};
