#include <chrono>
#include <iostream>
#include "vec3/vec3.hpp"

template<typename Func>
double benchmark(Func f, int n_iter = 1'000'000){
    auto start = std::chrono::high_resolution_clock::now();

    for(int i = 0; i<n_iter; i++) {
        f(); // call template function n_iter times
    }

    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::nano>(end - start).count() / n_iter;
}