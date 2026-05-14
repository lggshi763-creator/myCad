#include <mycad/domain/Vector3D.hpp>

#include <cmath>
#include <string>

namespace mycad::domain {

double Vector3D::length() const noexcept {
    return std::sqrt(x * x + y * y + z * z);
}

Vector3D Vector3D::normalized() const noexcept {
    const double len = length();
    if (len < kDefaultEpsilon)
        return Vector3D{0.0, 0.0, 0.0};
    return Vector3D{x / len, y / len, z / len};
}

std::string to_string(Vector3D v) {
    std::string out;
    out.reserve(64);
    out.append("Vector3D{x=");
    out.append(std::to_string(v.x));
    out.append(", y=");
    out.append(std::to_string(v.y));
    out.append(", z=");
    out.append(std::to_string(v.z));
    out.append("}");
    return out;
}

}  // namespace mycad::domain
