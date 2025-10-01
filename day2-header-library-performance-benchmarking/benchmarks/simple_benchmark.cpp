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

int main() {
    using Vec3f = Vec3<float>;
    
    // Test vectors
    Vec3f v1(1.0f, 2.0f, 3.0f);
    Vec3f v2(4.0f, 5.0f, 6.0f);
    float scalar = 2.5f;
    
    std::cout << "Vec3 Performance Benchmark\n";
    std::cout << "==========================\n\n";
    
    // Benchmark vector addition
    double add_time = benchmark([&]() {
        volatile auto result = v1 + v2;
        (void)result; // prevent optimization
    });
    std::cout << "Vector addition: " << add_time << " ns/op\n";
    
    // Benchmark vector subtraction
    double sub_time = benchmark([&]() {
        volatile auto result = v1 - v2;
        (void)result;
    });
    std::cout << "Vector subtraction: " << sub_time << " ns/op\n";
    
    // Benchmark scalar multiplication
    double mul_time = benchmark([&]() {
        volatile auto result = v1 * scalar;
        (void)result;
    });
    std::cout << "Scalar multiplication: " << mul_time << " ns/op\n";
    
    // Benchmark dot product
    double dot_time = benchmark([&]() {
        volatile auto result = v1.dot(v2);
        (void)result;
    });
    std::cout << "Dot product: " << dot_time << " ns/op\n";
    
    // Benchmark length calculation
    double length_time = benchmark([&]() {
        volatile auto result = v1.length();
        (void)result;
    });
    std::cout << "Length calculation: " << length_time << " ns/op\n";
    
    return 0;
}
