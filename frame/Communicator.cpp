//
// Created by taganyer on 2026/2/8.
//

#include "Communicator.hpp"

#include <cassert>

std::mutex Communicator::MUTEX;

std::vector<std::vector<int64_t> > Communicator::MATRIX;

std::map<Communicator::ID, std::shared_ptr<Communicator> > Communicator::COMMUNICATORS;

void Communicator::create_communicators(const std::vector<std::vector<int64_t> > &comm_matrix,
                                        const std::vector<tdcf::IdentityPtr> &targets) {
    assert(comm_matrix.size() == targets.size());
    assert(comm_matrix.size() && comm_matrix.size() == comm_matrix[0].size());
    MATRIX = comm_matrix;
    auto n = comm_matrix.size();
    for (tdcf::Identity::Uid i = 0; i < n; ++i) {
        COMMUNICATORS.emplace(i, std::make_shared<Communicator>(targets[i]));
    }
}

std::shared_ptr<Communicator> Communicator::get_communicator(tdcf::Identity::Uid id) {
    std::lock_guard l(MUTEX);
    auto iter = COMMUNICATORS.find(id);
    if (iter == COMMUNICATORS.end()) return nullptr;
    return iter->second;
}

int64_t Communicator::get_communication_ability(tdcf::Identity::Uid sender,
                                                tdcf::Identity::Uid receiver) {
    std::lock_guard l(MUTEX);
    return MATRIX[sender][receiver];
}

void Communicator::destroy_comm(tdcf::Identity::Uid id) {
    std::lock_guard l(MUTEX);
    COMMUNICATORS.erase(id);
    auto n = MATRIX.size();
    assert(id < n);

    for (uint32_t i = 0; i < n; ++i) {
        MATRIX[i][id] = -1;
        MATRIX[id][i] = -1;
    }
}

void Communicator::clear_comm() {
    std::lock_guard l(MUTEX);
    COMMUNICATORS.clear();
    MATRIX.clear();
}

Communicator::Communicator(tdcf::IdentityPtr id) : _id(std::move(id)) {}

bool Communicator::connect(const tdcf::IdentityPtr &target) {
    auto ptr = get_communicator(target->guid());
    if (!ptr) return false;

    ptr->connect_to_me(_id->guid(), this);

    std::unique_lock l(_private_mutex);
    _con.wait(l, [this, id = target->guid()] {
        return get_comm(id);
    });

    return true;
}

tdcf::IdentityPtr Communicator::accept() {
    std::unique_lock l(_private_mutex);
    ID id = -1;
    std::shared_ptr<std::atomic_bool> flag;
    _con.wait(l, [this, &id, &flag] {
        std::lock_guard ll(_mutex);
        if (!_connect.empty()) {
            id = _connect.front().first;
            flag = std::move(_connect.front().second);
            _connect.pop();
        }
        return id != -1;
    });

    auto ptr = get_communicator(id);
    assert(ptr);
    ptr->accept_to_me(_id->guid(), this, std::move(flag));

    return ptr->_id;
}

bool Communicator::disconnect(const tdcf::IdentityPtr &target) {
    auto ptr = get_comm(target->guid());
    assert(ptr);

    ptr->disconnect_from_me(_id->guid());

    std::unique_lock l(_private_mutex);
    _con.wait(l, [this, id = target->guid()] {
        std::lock_guard ll(_mutex);
        auto iter = _comm.find(id);
        assert(iter != _comm.end());
        return iter->second.disconnected;
    });

    std::lock_guard ll(_mutex);
    auto iter = _comm.find(target->guid());
    assert(iter->second.waiting_message == 0);
    _comm.erase(iter);
    return true;
}

tdcf::OperationFlag Communicator::send_message(const tdcf::IdentityPtr &target,
                                               const tdcf::Message &message,
                                               const tdcf::SerializablePtr &data) {
    auto ptr = get_comm(target->guid());
    assert(ptr);

    ptr->send_to_me(create_msg(target->guid(), message, data));

    return tdcf::OperationFlag::Success;
}

tdcf::OperationFlag Communicator::get_events(EventQueue &queue) {
    std::unique_lock l(_private_mutex);

    _con.wait_for(l, std::chrono::milliseconds(50), [this] {
        std::lock_guard ll(_mutex);
        return _recv_queue.size() > 2 && _recv_queue.top().work_time <= std::chrono::system_clock::now()
               || !_disconnect.empty();
    });

    auto size = queue.size();
    flush_msg(queue, std::chrono::system_clock::now());
    flush_disconnect(queue);

    return queue.size() != size ? tdcf::OperationFlag::Success : tdcf::OperationFlag::FurtherWaiting;
}

void Communicator::send_to_me(Msg msg) {
    std::lock_guard l(_mutex);
    auto iter = _comm.find(msg.sender->guid());
    assert(iter != _comm.end());
    ++iter->second.waiting_message;
    _recv_queue.emplace(std::move(msg));
    _con.notify_one();
}

void Communicator::connect_to_me(ID receiver, Communicator *recv_ptr) {
    std::lock_guard l(_mutex);
    auto flag = std::make_shared<std::atomic_bool>(false);
    _connect.emplace(receiver, flag);
    _comm.emplace(receiver, Data { recv_ptr, std::move(flag) });
    _con.notify_one();
}

void Communicator::accept_to_me(ID acceptor, Communicator *acc_ptr,
                                std::shared_ptr<std::atomic_bool> flag) {
    std::lock_guard l(_mutex);
    _comm.emplace(acceptor, Data { acc_ptr, std::move(flag) });
    _con.notify_one();
}

void Communicator::disconnect_from_me(ID sender) {
    std::lock_guard l(_mutex);
    auto iter = _comm.find(sender);
    assert(iter != _comm.end());
    if (iter->second.flag->exchange(true) == false) {
        _disconnect.emplace(sender);
    }
    iter->second.disconnected = true;
    _con.notify_one();
}

Communicator *Communicator::get_comm(ID id) {
    std::lock_guard l(_mutex);
    auto iter = _comm.find(id);
    if (iter == _comm.end()) return nullptr;
    return iter->second.ptr;
}

Communicator::Msg Communicator::create_msg(ID receiver, const tdcf::Message &message,
                                           const tdcf::SerializablePtr &data) {
    assert(receiver != _id->guid());
    std::lock_guard l(_mutex);
    TimeStamp last_time = std::max(_last_send_time, std::chrono::system_clock::now());

    auto ability = get_communication_ability(_id->guid(), receiver);
    assert(ability > 0);

    uint64_t bytes = message.serialize_size() + (data ? data->serialize_size() : 0);
    double cost_sec = (double) bytes / ability;
    auto cost_duration = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::duration<double>(cost_sec)
    );
    TimeStamp time = last_time + cost_duration;
    if (time <= last_time) {
        time = time + std::chrono::microseconds(1);
    }

    _last_send_time = time;
    return { time, _id, message, data };
}

void Communicator::flush_msg(EventQueue &queue, TimeStamp end_waiting) {
    std::lock_guard l(_mutex);
    while (!_recv_queue.empty() && _recv_queue.top().work_time <= end_waiting) {
        auto [time, sender, message, data] = _recv_queue.top();
        _recv_queue.pop();

        auto iter = _comm.find(sender->guid());
        assert(iter != _comm.end());
        --iter->second.waiting_message;

        queue.emplace(tdcf::CommunicatorEvent {
            tdcf::CommunicatorEvent::ReceivedMessage, std::move(sender),
            message.meta_data, std::move(data)
        });
    }
}

void Communicator::flush_disconnect(EventQueue &queue) {
    std::lock_guard l(_mutex);
    DisconnectQueue wait;
    while (!_disconnect.empty()) {
        auto iter = _comm.find(_disconnect.front());
        if (iter != _comm.end()) {
            if (iter->second.waiting_message == 0) {
                queue.emplace(tdcf::CommunicatorEvent {
                    tdcf::CommunicatorEvent::DisconnectRequest,
                    iter->second.ptr->_id,
                    tdcf::MetaData(),
                    nullptr
                });
            } else {
                wait.emplace(_disconnect.front());
            }
        }

        _disconnect.pop();
    }

    if (!wait.empty()) wait.swap(_disconnect);
}
