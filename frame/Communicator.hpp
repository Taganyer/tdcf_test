//
// Created by taganyer on 2026/2/8.
//

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <memory>
#include <queue>
#include <tdcf/frame/Communicator.hpp>

class Communicator : public tdcf::Communicator {
public:
    static void create_communicators(const std::vector<std::vector<int64_t> > &comm_matrix,
                                     const std::vector<tdcf::IdentityPtr> &targets);

    static std::shared_ptr<Communicator> get_communicator(tdcf::Identity::Uid id);

    // 每秒最多可以发送的字节
    static int64_t get_communication_ability(tdcf::Identity::Uid sender, tdcf::Identity::Uid receiver);

    static void destroy_comm(tdcf::Identity::Uid id);

    static void clear_comm();

    explicit Communicator(tdcf::IdentityPtr id);

    bool connect(const tdcf::IdentityPtr &target) override;

    tdcf::IdentityPtr accept() override;

    bool disconnect(const tdcf::IdentityPtr &target) override;

    tdcf::OperationFlag send_message(const tdcf::IdentityPtr &target,
                                     const tdcf::Message &message,
                                     const tdcf::SerializablePtr &data) override;

    tdcf::OperationFlag get_events(EventQueue &queue) override;

private:
    using ID = tdcf::Identity::Uid;

    static std::mutex MUTEX;

    static std::vector<std::vector<int64_t> > MATRIX;

    static std::map<ID, std::shared_ptr<Communicator> > COMMUNICATORS;

    using TimeStamp = std::chrono::system_clock::time_point;

    struct Msg {
        TimeStamp work_time;
        tdcf::IdentityPtr sender;
        tdcf::Message message;
        tdcf::SerializablePtr data;

        friend bool operator<(const Msg &a, const Msg &b) {
            return a.work_time > b.work_time;
        };
    };

    void send_to_me(Msg msg);

    void connect_to_me(ID receiver, Communicator *recv_ptr);

    void accept_to_me(ID acceptor, Communicator *acc_ptr, std::shared_ptr<std::atomic_bool> flag);

    void disconnect_from_me(ID sender);

    Communicator *get_comm(ID id);

    Msg create_msg(ID receiver, const tdcf::Message &message,
                   const tdcf::SerializablePtr &data);

    void flush_msg(EventQueue &queue, TimeStamp end_waiting);

    void flush_disconnect(EventQueue &queue);

    tdcf::IdentityPtr _id;

    TimeStamp _last_send_time;

    std::mutex _mutex;

    std::condition_variable _con;

    using MessageQueue = std::priority_queue<Msg>;

    MessageQueue _recv_queue;

    using ConnectQueue = std::queue<std::pair<ID, std::shared_ptr<std::atomic_bool>>>;

    ConnectQueue _connect;

    using DisconnectQueue = std::queue<ID>;

    DisconnectQueue _disconnect;

    // private lock
    std::mutex _private_mutex;

    struct Data {
        Communicator *ptr;
        // 用于标记哪一方主动断开
        std::shared_ptr<std::atomic_bool> flag;
        uint64_t waiting_message = 0;
        bool disconnected = false;
    };

    std::map<ID, Data> _comm;
};
