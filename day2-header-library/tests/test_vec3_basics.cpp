#include <vec3/vec3.hpp>
#include <iostream>
#include <cassert>
#include <cmath>

int main() {
    using namespace vec3;
    
    // Test basic construction
    Vec3 v1(1.0, 2.0, 3.0);
    Vec3 v2(4.0, 5.0, 6.0);
    
    // Test addition
    Vec3 sum = v1 + v2;
    assert(sum.x == 5.0 && sum.y == 7.0 && sum.z == 9.0);
    
    // Test dot product
    double dot_result = v1.dot(v2);
    assert(dot_result == 32.0); // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
    
    // Test scalar multiplication
    Vec3 scaled = v1 * 2.0;
    assert(scaled.x == 2.0 && scaled.y == 4.0 && scaled.z == 6.0);
    
    // Test length calculation
    Vec3 unit_x(1.0, 0.0, 0.0);
    assert(std::abs(unit_x.length() - 1.0) < 1e-10);
    
    // Test normalization
    Vec3 normalized = v1.normalized();
    assert(std::abs(normalized.length() - 1.0) < 1e-10);
    
    std::cout << "All tests passed!" << std::endl;
    std::cout << "v1: " << v1 << std::endl;
    std::cout << "v2: " << v2 << std::endl;
    std::cout << "v1 + v2: " << sum << std::endl;
    
    return 0;
} 