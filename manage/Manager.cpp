//
// Created by taganyer on 2026/2/10.
//

#include "Manager.hpp"
#include "../frame/ProcessingRules.hpp"
#include "../frame/Communicator.hpp"

#include "../log.hpp"
#include <tdcf/base/Errors.hpp>
#include <tdcf/cluster/DBT/DBTCluster.hpp>
#include <tdcf/cluster/ring/RingCluster.hpp>
#include <tdcf/cluster/star/StarCluster.hpp>

using namespace std;


Manager::Manager(tdcf::StageNum topo_type, int64_t intra, int64_t inter) :
    _type(topo_type), _intra(intra), _inter(inter) {}

Manager::~Manager() {
    if (_thread.joinable()) {
        _thread.join();
    }
}

void Manager::add_operation(tdcf::OperationType type, RulePtr rule) {
    _operations.emplace_back(type, std::move(rule));
}

void Manager::add_sub_cluster(ManagerPtr sub) {
    _total += sub->_total;
    _subs.emplace_back(std::move(sub));
}

void Manager::root_init() {
    uint32_t id = 0;
    std::vector comm_matrix(_total, vector<int64_t>(_total, -1));
    std::vector<tdcf::IdentityPtr> targets;

    _id = std::make_shared<Identity>(id, Identity::MOD::ROOT, _total);
    _processor = std::make_shared<Processor>(_id);
    targets.push_back(_id);
    comm_matrix[id][id] = _intra;

    ++id;
    for (auto &sub: _subs) {
        sub->_root = _id;
        sub->init(id, comm_matrix, targets);
    }

    for (auto &sub: _subs) {
        comm_matrix[_id->guid()][sub->_id->guid()] = sub->_total == 1 ? _intra : _inter;
        comm_matrix[sub->_id->guid()][_id->guid()] = sub->_total == 1 ? sub->_intra : sub->_inter;
    }

    for (uint32_t i = 0; i < _subs.size(); ++i) {
        for (int j = 0; j < _subs.size(); ++j) {
            if (i == j) continue;
            bool is_intra = _subs[i]->_total == 1 && _subs[j]->_total == 1;
            comm_matrix[_subs[i]->_id->guid()][_subs[j]->_id->guid()] = is_intra ? _subs[i]->_intra : _subs[i]->_inter;
        }
    }

    Communicator::create_communicators(comm_matrix, targets);
}

void Manager::start() {
    assert(!_running);
    for (auto &sub: _subs) {
        sub->start();
    }
    _thread = std::thread([this] {
        _running.store(true);
        if (_id->guid() == 0) {
            root();
        } else if (!_subs.empty()) {
            node_root();
        } else {
            pure_node();
        }
        _running.store(false);
    });
}

void Manager::create_tasks(tdcf::Cluster &root) {
    for (auto &[type, rule]: _operations) {
        rule->set_callback([this] { _tasks_size.fetch_sub(1); });
        auto flag = tdcf::StatusFlag::EventEnd;
        switch (type) {
            case tdcf::OperationType::Broadcast:
                flag = root.broadcast(rule);
                break;
            case tdcf::OperationType::Scatter:
                flag = root.scatter(rule);
                break;
            case tdcf::OperationType::Reduce:
                flag = root.reduce(rule);
                break;
            case tdcf::OperationType::AllReduce:
                flag = root.all_reduce(rule);
                break;
            case tdcf::OperationType::ReduceScatter:
                flag = root.reduce_scatter(rule);
                break;
            default:
                break;
        }
        TDCF_CHECK_SUCCESS(flag)
        _tasks_size.fetch_add(1);
    }
}

std::shared_ptr<tdcf::Cluster> Manager::create_cluster() {
    auto comm = Communicator::get_communicator(_id->guid());
    if (_type == tdcf::ClusterType::star) {
        return std::make_shared<tdcf::StarCluster>(_id, std::move(comm), _processor);
    }
    if (_type == tdcf::ClusterType::ring) {
        return std::make_shared<tdcf::RingCluster>(_id, std::move(comm), _processor);
    }
    if (_type == tdcf::ClusterType::dbt) {
        return std::make_shared<tdcf::DBTCluster>(_id, std::move(comm), _processor);
    }
    TDCF_RAISE_ERROR(error cluster type)
}

tdcf::Node Manager::create_node() {
    auto comm = Communicator::get_communicator(_id->guid());
    return { _id, std::move(comm), _processor };
}

void Manager::root() {
    auto root = create_cluster();

    tdcf::Cluster::IdentitySet set;
    for (auto &i: _subs) {
        set.emplace(i->_id);
    }

    root->start_cluster(set, false);

    ERROR("%s ROOT[%u] start", __PRETTY_FUNCTION__, _id->guid());

    auto flag = tdcf::StatusFlag::Success;

    create_tasks(*root);

    while (flag == tdcf::StatusFlag::Success && _tasks_size.load() > 0) {
        flag = root->handle_a_loop();
    }

    ERROR("%s ROOT[%u] start end_cluster: %s", __PRETTY_FUNCTION__, _id->guid(), tdcf::status_flag_name(flag));
    flag = root->end_cluster();
    ERROR("%s ROOT[%u] end: %s", __PRETTY_FUNCTION__, _id->guid(), tdcf::status_flag_name(flag));
}

void Manager::node_root() {
    auto node_root = create_cluster();

    tdcf::Cluster::IdentitySet set;
    for (auto &i: _subs) {
        set.emplace(i->_id);
    }

    node_root->start_cluster(set, true);

    ERROR("%s NODE_ROOT[%u] start", __PRETTY_FUNCTION__, _id->guid());

    auto flag = tdcf::StatusFlag::Success;

    create_tasks(*node_root);

    bool root_end = false;
    while (flag == tdcf::StatusFlag::Success && (_tasks_size.load() > 0 || !root_end)) {
        flag = node_root->handle_a_loop();
        if (flag == tdcf::StatusFlag::ClusterOffline) {
            root_end = true;
            flag = tdcf::StatusFlag::Success;
        }
    }

    ERROR("%s NODE_ROOT[%u] start end_cluster: %s", __PRETTY_FUNCTION__, _id->guid(), tdcf::status_flag_name(flag));
    flag = node_root->end_cluster();
    ERROR("%s NODE_ROOT[%u] end: %s", __PRETTY_FUNCTION__, _id->guid(), tdcf::status_flag_name(flag));
}

void Manager::pure_node() {
    auto node = create_node();
    node.start_node();
    ERROR("%s NODE[%u] start", __PRETTY_FUNCTION__, _id->guid());

    auto flag = tdcf::StatusFlag::Success;
    while (flag == tdcf::StatusFlag::Success) {
        flag = node.handle_a_loop();
    }

    ERROR("%s NODE[%u] end: %s", __PRETTY_FUNCTION__, _id->guid(), tdcf::status_flag_name(flag));
}

void Manager::init(uint32_t &id, std::vector<std::vector<int64_t> > &comm_matrix,
                   std::vector<tdcf::IdentityPtr> &targets) {
    _id = std::make_shared<Identity>(id, _total == 1 ? Identity::MOD::NODE : Identity::MOD::NODE_ROOT, _total);
    _processor = std::make_shared<Processor>(_id);
    targets.push_back(_id);
    comm_matrix[id][id] = _intra;

    ++id;
    for (auto &sub: _subs) {
        sub->_root = _id;
        sub->init(id, comm_matrix, targets);
    }

    for (auto &sub: _subs) {
        comm_matrix[_id->guid()][sub->_id->guid()] = sub->_total == 1 ? _intra : _inter;
        comm_matrix[sub->_id->guid()][_id->guid()] = sub->_total == 1 ? sub->_intra : sub->_inter;
    }

    for (uint32_t i = 0; i < _subs.size(); ++i) {
        for (int j = 0; j < _subs.size(); ++j) {
            if (i == j) continue;
            bool is_intra = _subs[i]->_total == 1 && _subs[j]->_total == 1;
            comm_matrix[_subs[i]->_id->guid()][_subs[j]->_id->guid()] = is_intra ? _subs[i]->_intra : _subs[i]->_inter;
        }
    }
}
