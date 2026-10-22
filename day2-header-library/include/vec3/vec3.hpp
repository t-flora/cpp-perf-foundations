#pragma once
// Day 2: a header-only, constexpr 3D vector. Fill in the TODOs.

namespace vec3 {

template <typename T>
class Vec3 {
public:
    // TODO: constructors: default (zero), (x, y, z), and an explicit one filling all three

    // TODO: accessors x(), y(), z(). How do you allow writes and still support const Vec3?

    // TODO: arithmetic: +, -, unary -, * scalar, / scalar, and +=, -=, *=, /=
    // TODO: ==
    // TODO: dot, cross, length_squared, length, normalized
    // TODO: operator<< for printing

private:
    T x_{}, y_{}, z_{};
};

// TODO: scalar * vector, as a free function. Make `2 * Vec3<double>{}` compile.

} // namespace vec3
