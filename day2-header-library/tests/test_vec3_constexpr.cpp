// Compile-time tests: if this file compiles, every check passed.
#include <vec3/vec3.hpp>
#include <type_traits>

using vec3::Vec3;

// Rule of zero keeps Vec3 trivially copyable: safe to memcpy, cheap to pass around
static_assert(std::is_trivially_copyable_v<Vec3<float>>);
static_assert(sizeof(Vec3<float>) == 3 * sizeof(float));

constexpr Vec3 a(1, 2, 3);
constexpr Vec3 b(4, 5, 6);

static_assert(a + b == Vec3(5, 7, 9));
static_assert(b - a == Vec3(3));
static_assert(-a == Vec3(-1, -2, -3));
static_assert(a * 2 == Vec3(2, 4, 6));
static_assert(2 * a == Vec3(2, 4, 6));
static_assert(Vec3(2, 4, 6) / 2 == a);
static_assert(a.dot(b) == 32);
static_assert(a.cross(b) == Vec3(-3, 6, -3));
static_assert(Vec3(3, 4, 12).length_squared() == 169);

// Compound assignment works in constant expressions too
constexpr Vec3<int> accumulate() {
    Vec3<int> v;
    for (int i = 1; i <= 4; ++i) {
        v += Vec3(i);
    }
    v *= 2;
    return v;
}
static_assert(accumulate() == Vec3(20));

// Division by zero throws, and a throw is not a constant expression, so this would not
// compile (uncomment to see the error):
// static_assert(a / 0 == a);

int main() { return 0; }
