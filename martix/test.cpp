//
// Created by taganyer on 2026/2/10.
//
#include "test.hpp"
#include "Matrix.hpp"
#include <chrono>
#include <fstream>
#include <thread>
#include "../log.hpp"
#include "MartixRules.hpp"
#include "../manage/Manager.hpp"


using namespace std;

template<typename Func>
static double measure_time_ms(Func &&func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

static std::pair<MARTIX, MARTIX> get_random_martix(int N) {
    generate_random_matrix<MARTIX_TYPE>(PROJECT_PATH "/resource/matrix1.txt", N, N, 0, 10);
    generate_random_matrix<MARTIX_TYPE>(PROJECT_PATH "/resource/matrix2.txt", N, N, 0, 10);

    auto in1 = ifstream(PROJECT_PATH "/resource/matrix1.txt"),
            in2 = ifstream(PROJECT_PATH "/resource/matrix2.txt");

    return std::pair<MARTIX, MARTIX>(in1, in2);
}

static ManagerPtr create_manager(tdcf::StageNum topo_type, int64_t intra, int64_t inter, int nodes) {
    auto ptr = std::make_unique<Manager>(topo_type, intra, inter);

    for (int i = 0; i < nodes; ++i) {
        auto sub = std::make_unique<Manager>(topo_type, intra, inter);
        ptr->add_sub_cluster(std::move(sub));
    }

    return ptr;
}

static void add_operation(Manager &manage, uint64_t &id, MARTIX a, MARTIX b,
                          MARTIX *ans, TimePoint *start, TimePoint *end, double *cost) {
    auto re_rule = std::make_shared<ReduceMartixRules>(++id, [ans, start, end, cost](MARTIX m) {
        *end = std::chrono::high_resolution_clock::now();
        *cost = std::chrono::duration<double, std::milli>(*end - *start).count();
        *ans = std::move(m);
    });

    auto sc_rule = std::make_shared<ScatterMartixRules>(++id, std::move(a), std::move(b), start, re_rule.get());

    manage.add_operation(tdcf::OperationType::Scatter, std::move(sc_rule));
    manage.add_operation(tdcf::OperationType::Reduce, std::move(re_rule));
}

static const char *get_topo_name(tdcf::StageNum topo_type) {
    constexpr const char *names[] {
        "Star",
        "Ring",
        "DBT",
    };

    return names[topo_type - 1];
}

void distribution_SUMMA2D_test(int N, int nodes, int times, bool test) {
    assert(N % (nodes + 1) == 0);

    vector<vector<double> > costs(times);

    auto [fst, snd] = get_random_martix(N);

    auto &a = fst, &b = snd;
    MARTIX ans1;

    if (test) {
        auto cost1 = measure_time_ms([&] {
           ans1 = a * b;
       });

        INFO("common multiply result size:[%lu*%lu] cost %.02lf ms", ans1.row(), ans1.col(), cost1);
    }

    for (int i = 0; i < times; ++i) {
        MARTIX ans2;

        auto cost2 = measure_time_ms([&] {
            auto [fst_, snd_] = SUMMA2D_scatter(a, b, nodes + 1);

            auto &left = fst_, &right = snd_;

            MARTIX::Matrix_Array_3D obj(nodes + 1);

            vector<thread> threads;

            for (int j = 0; j < nodes + 1; ++j) {
                threads.emplace_back([&, j] {
                    obj[j] = SUMMA2D_multiply(left[j], right[j]);
                });
            }

            for (auto &t: threads) t.join();

            ans2 = merge(SUMMA2D_reduce(obj));
        });
        if (test) assert(compare(ans1, ans2));

        costs[i].push_back(cost2);
        INFO("thread multiply result size:[%lu*%lu] cost %.02lf ms", ans2.row(), ans2.col(), cost2);

        for (tdcf::StageNum topo_type = tdcf::ClusterType::star; topo_type <= tdcf::ClusterType::dbt; ++topo_type) {
            MARTIX ans3;
            uint64_t rule_id = 0;
            TimePoint start, end;
            double cost3;

            auto manager = create_manager(topo_type, 1LL << 33, 1LL << 32, nodes);

            add_operation(*manager, rule_id, a.copy(), b.copy(), &ans3, &start, &end, &cost3);

            manager->root_init();
            INFO("distribution_SUMMA2D_test: %s total %d threads.", get_topo_name(topo_type), nodes + 1);

            manager->start();

            manager.reset();
            Communicator::clear_comm();

            if (test) assert(compare(ans2, ans3));

            INFO("distribution_SUMMA2D result size:[%lu*%lu] cost %.02lf ms", ans3.row(), ans3.col(), cost3);
            costs[i].push_back(cost3);
        }
    }

    vector<double> avg(costs[0].size());
    for (auto &v: costs) {
        for (int i = 0; i < v.size(); ++i) {
            avg[i] += v[i];
        }
    }
    for (auto &v: avg) {
        v /= times;
    }

    assert(avg.size() == 4);
    FATAL("\nmartix{(%d*%d) * (%d*%d)}\n"
         "%d threads:(%.02fms)\n"
         "%d TDCF Star nodes:(%.02fms)\n"
         "%d TDCF Ring nodes:(%.02fms)\n"
         "%d TDCF DBT nodes:(%.02fms)",
         N, N, N, N,
         nodes + 1, avg[0],
         nodes + 1, avg[1],
         nodes + 1, avg[2],
         nodes + 1, avg[3]);
}
