#include <mycad/domain/Point3D.hpp>

#include <string>

namespace mycad::domain {

std::string to_string(Point3D p) {
    std::string out;
    out.reserve(64);
    out.append("Point3D{x=");
    out.append(std::to_string(p.x));
    out.append(", y=");
    out.append(std::to_string(p.y));
    out.append(", z=");
    out.append(std::to_string(p.z));
    out.append("}");
    return out;
}

}  // namespace mycad::domain
