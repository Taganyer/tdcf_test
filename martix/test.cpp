//
// Created by taganyer on 2026/2/10.
//
#include "test.hpp"
#include "Matrix.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
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

void SUMMA2D_test() {
    using Type = int;
    constexpr int N = 1000;
    generate_random_matrix<Type>(PROJECT_PATH "/resource/matrix1.txt", N, N, 0, 10);
    generate_random_matrix<Type>(PROJECT_PATH "/resource/matrix2.txt", N, N, 0, 10);

    auto in1 = ifstream("../resource/matrix1.txt"), in2 = ifstream("../resource/matrix2.txt");

    Matrix<Type> a(in1), b(in2);

    in1.close();
    in2.close();


    Matrix<Type> ans1, ans2;

    auto cost1 = measure_time_ms([&] {
        ans1 = a * b;
    });

    INFO("flag1: %.02lf ms", cost1);
    // ans1.show(cout);

    auto cost2 = measure_time_ms([&] {
        constexpr int n = 5;

        auto [fst, snd] = SUMMA2D_scatter(a, b, n);

        auto &left = fst, &right = snd;

        Matrix<Type>::Matrix_Array_3D obj(n);

        vector<thread> threads;

        for (int i = 0; i < n; ++i) {
            threads.emplace_back([&, i] {
                obj[i] = SUMMA2D_multiply(left[i], right[i]);
            });
        }

        for (auto &t: threads) t.join();

        ans2 = merge(SUMMA2D_reduce(obj));
    });

    // ans2.show(cout);
    INFO("flag2: %.02lf ms", cost2);

    INFO("%s", (compare(ans1, ans2) ? "true" : "false"));
}

using TimePoint = decltype(std::chrono::high_resolution_clock::now());

static ManagerPtr create(uint64_t &id, MARTIX a, MARTIX b,
                         const MARTIX *result, TimePoint *start,
                         tdcf::StageNum topo_type, int64_t intra, int64_t inter, int nodes) {
    auto ptr = std::make_unique<Manager>(topo_type, intra, inter);

    auto re_rule = std::make_shared<ReduceMartixRules>(++id, [result, start](MARTIX m) {
        auto end = std::chrono::high_resolution_clock::now();
        double cost = std::chrono::duration<double, std::milli>(end - *start).count();

        auto cmp = compare(m, *result);
        if (!cmp) {
            auto out = ofstream(PROJECT_PATH "/resource/error.txt");
            ERROR("distribution_SUMMA2D wrong ans cost %.02lf ms", cost);
        } else {
            INFO("distribution_SUMMA2D cost %.02lf ms", cost);
        }
    });

    auto sc_rule = std::make_shared<ScatterMartixRules>(++id, std::move(a), std::move(b), re_rule.get());
    ptr->add_operation(tdcf::OperationType::Scatter, std::move(sc_rule));
    ptr->add_operation(tdcf::OperationType::Reduce, std::move(re_rule));

    for (int i = 0; i < nodes; ++i) {
        auto sub = std::make_unique<Manager>(topo_type, intra, inter);
        ptr->add_sub_cluster(std::move(sub));
    }

    return ptr;
}

void distribution_SUMMA2D_test() {
    constexpr int N = 1000;
    generate_random_matrix<MARTIX_TYPE>(PROJECT_PATH "/resource/matrix1.txt", N, N, 0, 10);
    generate_random_matrix<MARTIX_TYPE>(PROJECT_PATH "/resource/matrix2.txt", N, N, 0, 10);
    auto in1 = ifstream("../resource/matrix1.txt"), in2 = ifstream("../resource/matrix2.txt");

    Matrix<MARTIX_TYPE> a(in1), b(in2);

    in1.close();
    in2.close();

    Matrix<MARTIX_TYPE> ans1;

    auto cost1 = measure_time_ms([&] {
        ans1 = a * b;
    });

    INFO("common multiply cost %.02lf ms", cost1);

    uint64_t rule_id = 0;
    TimePoint start;
    auto star = create(rule_id, std::move(a), std::move(b), &ans1, &start,
                       tdcf::ClusterType::star, 1LL << 33, 1LL << 32, 4);
    star->root_init();

    start = std::chrono::high_resolution_clock::now();
    star->start();

    star.reset();
    Communicator::clear_comm();
}
