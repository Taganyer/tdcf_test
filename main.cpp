#include <chrono>
#include <iostream>
#include <thread>
#include <fstream>

#include "Matrix.hpp"

using namespace std;


template<typename Func>
double measure_time_ms(Func &&func) {
    auto start = std::chrono::high_resolution_clock::now();
    func(); // 执行传入的函数
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}


using Type = int;

int main() {
    constexpr int N = 1000;
    generate_random_matrix<Type>("../matrix3.txt", N, N, 0, 10);
    generate_random_matrix<Type>("../matrix4.txt", N, N, 0, 10);

    auto in1 = ifstream("../matrix3.txt"), in2 = ifstream("../matrix4.txt");

    Matrix<Type> a(in1), b(in2);

    in1.close();
    in2.close();


    Matrix<Type> ans1, ans2;

    auto cost1 = measure_time_ms([&] {
        ans1 = a * b;
    });

    cout << "flag1: " << cost1 << "ms" << endl;
    // ans1.show(cout);

    auto cost2 = measure_time_ms([&] {
        constexpr int n = 5;

        auto as = a.split(n), bs = b.split(n);

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

        ans2 = SUMMA2D_reduce(obj);
    });

    // ans2.show(cout);
    cout << "flag2: " << cost2 << "ms" << endl;

    cout << std::boolalpha << compare(ans1, ans2) << endl;

    return 0;
}
