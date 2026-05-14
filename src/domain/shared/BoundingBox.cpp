#include <mycad/domain/BoundingBox.hpp>

#include <string>

namespace mycad::domain {

std::string to_string(const BoundingBox& bb) {
    std::string out;
    out.reserve(128);
    out.append("BoundingBox{min=");
    out.append(to_string(bb.min));
    out.append(", max=");
    out.append(to_string(bb.max));
    out.append("}");
    return out;
}

}  // namespace mycad::domain
