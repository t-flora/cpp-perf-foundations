#undef NDEBUG // keep assert() active even in Release builds
#include <vec3/vec3.hpp>
#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>

using vec3::Vec3;
using vec3::Vec3d;

static bool approx(double a, double b, double eps = 1e-10) { return std::abs(a - b) < eps; }

template <typename Fn>
static bool throws_invalid_argument(Fn fn) {
    try { fn(); } catch (const std::invalid_argument&) { return true; }
    return false;
}

int main() {
    // Test basic construction (CTAD deduces Vec3<double>)
    Vec3 v1(1.0, 2.0, 3.0);
    Vec3 v2(4.0, 5.0, 6.0);
    assert(Vec3d() == Vec3d(0.0, 0.0, 0.0));
    assert(Vec3d(2.0) == Vec3d(2.0, 2.0, 2.0));

    // Accessors are writable
    Vec3d w;
    w.x() = 1.0; w.y() = 2.0; w.z() = 3.0;
    assert(w == v1);

    // Test addition, subtraction, negation
    Vec3 sum = v1 + v2;
    assert(sum.x() == 5.0 && sum.y() == 7.0 && sum.z() == 9.0);
    assert(v2 - v1 == Vec3d(3.0));
    assert(-v1 == Vec3d(-1.0, -2.0, -3.0));

    // Test dot product
    double dot_result = v1.dot(v2);
    assert(dot_result == 32.0); // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32

    // Test cross product: right-handed, orthogonal to both inputs
    const Vec3d x(1, 0, 0), y(0, 1, 0), z(0, 0, 1);
    assert(x.cross(y) == z && y.cross(z) == x && z.cross(x) == y);
    assert(v1.cross(v2) == Vec3d(-3.0, 6.0, -3.0));
    assert(v1.cross(v2).dot(v1) == 0.0 && v1.cross(v2).dot(v2) == 0.0);

    // Test scalar multiplication from both sides, and with an int scalar
    Vec3 scaled = v1 * 2.0;
    assert(scaled.x() == 2.0 && scaled.y() == 4.0 && scaled.z() == 6.0);
    assert(2.0 * v1 == scaled);
    assert(2 * v1 == scaled);
    assert(scaled / 2.0 == v1);

    // Compound assignment
    Vec3d acc = v1;
    acc += v2;  assert(acc == sum);
    acc -= v2;  assert(acc == v1);
    acc *= 3.0; assert(acc == Vec3d(3.0, 6.0, 9.0));
    acc /= 3.0; assert(acc == v1);

    // Test length calculation
    Vec3 unit_x(1.0, 0.0, 0.0);
    assert(approx(unit_x.length(), 1.0));
    assert(approx(Vec3d(3.0, 4.0, 12.0).length(), 13.0));

    // Test normalization
    Vec3 normalized = v1.normalized();
    assert(approx(normalized.length(), 1.0));

    // Errors
    assert(throws_invalid_argument([&] { (void)(v1 / 0.0); }));
    assert(throws_invalid_argument([&] { acc /= 0.0; }));
    assert(throws_invalid_argument([] { (void)Vec3d().normalized(); }));

    // Integer vectors work too (no length/normalize precision guarantees)
    assert(Vec3(1, 2, 3).dot(Vec3(4, 5, 6)) == 32);

    // Stream output
    std::ostringstream os;
    os << v1;
    assert(os.str() == "Vec3(1, 2, 3)");

    std::cout << "All tests passed!" << std::endl;
    std::cout << "v1: " << v1 << std::endl;
    std::cout << "v2: " << v2 << std::endl;
    std::cout << "v1 + v2: " << sum << std::endl;

    return 0;
}
