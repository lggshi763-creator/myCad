#include <mycad/domain/Point2D.hpp>

#include <string>

namespace mycad::domain {

std::string to_string(Point2D p) {
    std::string out;
    out.reserve(48);
    out.append("Point2D{x=");
    out.append(std::to_string(p.x));
    out.append(", y=");
    out.append(std::to_string(p.y));
    out.append("}");
    return out;
}

}  // namespace mycad::domain
