#pragma once
#include <ostream>
using namespace std;
// core vector class
template<typename T>
class Vec3 {
    private:
        T x_, y_, z_; // coordinates
    public:
        // all methods can be constexpr for the case where the method may be evaluated at
        // compile time
        constexpr Vec3() = default;
        constexpr Vec3(T x, T y, T z): x_(x), y_(y), z_(z) {}
        explicit constexpr Vec3(T val): x_(val), y_(val), z_(val) {};

        constexpr T& x() { return x_; }
        constexpr T& y() { return y_; }
        constexpr T& z() { return z_; }

        // for const correctness!
        constexpr const T& x() const { return x_; }
        constexpr const T& y() const { return y_; }
        constexpr const T& z() const { return z_; }

        constexpr T set_x(T x) { x_ = x; }
        constexpr T set_y(T y) { y_ = y; }
        constexpr T set_z(T z) { z_ = z; }

        constexpr Vec3 operator=(const Vec3& other) {
            if (&other != this){
                set_x(other.x());
                set_y(other.y());
                set_z(other.z());
            }
            return *this;
        }
        
        // arithmetic operations & assignments
        constexpr Vec3 operator+(const Vec3& other) const {
            return Vec3(x() + other.x(), y() + other.y(), z() + other.z());
        }
        constexpr Vec3 operator-(const Vec3& other) const {
            return Vec3(x() - other.x(), y() - other.y(), z() - other.z());
        }
        constexpr Vec3 operator*(T scalar) const {
            return Vec3(x() * scalar, y() * scalar, z() * scalar);
        }
        constexpr Vec3 operator/(T scalar) const {
            if(scalar == T(0)) {
                throw std::invalid_argument("Error: Vec3 divided by 0");
            }
            return Vec3(x() / scalar, y() / scalar, z() / scalar);
            
        }
        constexpr T dot(const Vec3& other) const {
            return x()*other.x() + y()*other.y() + z()*other.z();
        }

        constexpr Vec3& operator+=(const Vec3& other) {
            set_x(x() + other.x());
            set_y(y() + other.y());
            set_z(z() + other.z());
            return *this;
        }
        constexpr Vec3& operator-=(const Vec3& other) {
            set_x(x() - other.x());
            set_y(y() - other.y());
            set_z(z() - other.z());
            return *this;
        }
        constexpr Vec3& operator*=(T scalar) {
            set_x(x() * scalar);
            set_y(y() * scalar);
            set_z(z() * scalar);
            return *this;
        }
        constexpr Vec3& operator/=(T scalar) {
            if(scalar == T(0)) {
                throw std::invalid_argument("Error: Vec3 divided by 0");
            }
            set_x(x() / scalar);
            set_y(y() / scalar);
            set_z(z() / scalar);
            return *this;
        }

        friend ostream& operator<<(ostream& os, const Vec3& v) {
            os << "Vec3(" << v.x() << ", " << v.y() << ", " << v.z() << ")";
            return os;
        }

        constexpr T length() const {
            return std::sqrt(x()*x() + y()*y() + z()*z());
        }

        constexpr Vec3 normalized() {
            if(length() == T(0)){
                throw std::invalid_argument("Error: Cannot normalize zero vector");
            }
            return Vec3(x(), y(), z())/length();
        }
};
