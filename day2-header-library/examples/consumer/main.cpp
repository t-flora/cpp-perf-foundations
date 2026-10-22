#include <vec3/vec3.hpp>
#include <iostream>

int main() {
    using vec3::Vec3d;

    const Vec3d x(1.0, 0.0, 0.0);
    const Vec3d y(0.0, 1.0, 0.0);
    const Vec3d z = x.cross(y);

    std::cout << x << " x " << y << " = " << z << "\n";
    return z == Vec3d(0.0, 0.0, 1.0) ? 0 : 1;
}
