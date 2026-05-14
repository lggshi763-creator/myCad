#include <mycad/domain/Axis3D.hpp>

#include <string>

namespace mycad::domain {

std::string to_string(const Axis3D& a) {
    std::string out;
    out.reserve(128);
    out.append("Axis3D{origin=");
    out.append(to_string(a.origin));
    out.append(", dir=");
    out.append(to_string(a.direction));
    out.append("}");
    return out;
}

}  // namespace mycad::domain
