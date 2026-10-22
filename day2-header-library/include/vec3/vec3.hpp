#pragma once
#include <cmath>
#include <ostream>
#include <stdexcept>
#include <type_traits>

namespace vec3 {

// core vector class
template<typename T>
class Vec3 {
    static_assert(std::is_arithmetic_v<T>, "Vec3<T> requires an arithmetic type");

    private:
        T x_{}, y_{}, z_{}; // coordinates, zero-initialized by default
    public:
        // all methods are constexpr so they can be evaluated at compile time
        constexpr Vec3() = default;
        constexpr Vec3(T x, T y, T z): x_(x), y_(y), z_(z) {}
        explicit constexpr Vec3(T val): x_(val), y_(val), z_(val) {}

        // reference accessors double as setters: v.x() = 3;
        constexpr T& x() { return x_; }
        constexpr T& y() { return y_; }
        constexpr T& z() { return z_; }

        // for const correctness!
        constexpr const T& x() const { return x_; }
        constexpr const T& y() const { return y_; }
        constexpr const T& z() const { return z_; }

        // no user-declared copy/move/destructor: the compiler-generated ones are
        // correct, trivial, and keep Vec3 usable in constant expressions (rule of zero)

        // arithmetic operations & assignments
        constexpr Vec3 operator+(const Vec3& other) const {
            return Vec3(x_ + other.x_, y_ + other.y_, z_ + other.z_);
        }
        constexpr Vec3 operator-(const Vec3& other) const {
            return Vec3(x_ - other.x_, y_ - other.y_, z_ - other.z_);
        }
        constexpr Vec3 operator-() const {
            return Vec3(-x_, -y_, -z_);
        }
        constexpr Vec3 operator*(T scalar) const {
            return Vec3(x_ * scalar, y_ * scalar, z_ * scalar);
        }
        constexpr Vec3 operator/(T scalar) const {
            if (scalar == T(0)) {
                throw std::invalid_argument("Error: Vec3 divided by 0");
            }
            return Vec3(x_ / scalar, y_ / scalar, z_ / scalar);
        }

        constexpr Vec3& operator+=(const Vec3& other) {
            x_ += other.x_; y_ += other.y_; z_ += other.z_;
            return *this;
        }
        constexpr Vec3& operator-=(const Vec3& other) {
            x_ -= other.x_; y_ -= other.y_; z_ -= other.z_;
            return *this;
        }
        constexpr Vec3& operator*=(T scalar) {
            x_ *= scalar; y_ *= scalar; z_ *= scalar;
            return *this;
        }
        constexpr Vec3& operator/=(T scalar) {
            return *this = *this / scalar;
        }

        constexpr bool operator==(const Vec3&) const = default;

        constexpr T dot(const Vec3& other) const {
            return x_ * other.x_ + y_ * other.y_ + z_ * other.z_;
        }
        constexpr Vec3 cross(const Vec3& other) const {
            return Vec3(y_ * other.z_ - z_ * other.y_,
                        z_ * other.x_ - x_ * other.z_,
                        x_ * other.y_ - y_ * other.x_);
        }

        // length_squared is constexpr; length is not, because std::sqrt only
        // becomes constexpr in C++26
        constexpr T length_squared() const { return dot(*this); }
        T length() const { return static_cast<T>(std::sqrt(length_squared())); }

        Vec3 normalized() const {
            const T len = length();
            if (len == T(0)) {
                throw std::invalid_argument("Error: Cannot normalize zero vector");
            }
            return *this / len;
        }

        friend std::ostream& operator<<(std::ostream& os, const Vec3& v) {
            return os << "Vec3(" << v.x_ << ", " << v.y_ << ", " << v.z_ << ")";
        }
};

// scalar * vector. type_identity_t stops `2 * Vec3<double>{}` from failing
// deduction (int vs. double): T is deduced from the vector only
template<typename T>
constexpr Vec3<T> operator*(std::type_identity_t<T> scalar, const Vec3<T>& v) {
    return v * scalar;
}

using Vec3f = Vec3<float>;
using Vec3d = Vec3<double>;

} // namespace vec3
