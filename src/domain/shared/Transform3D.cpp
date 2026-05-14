#include <mycad/domain/Transform3D.hpp>

#include <string>

namespace mycad::domain {

Transform3D Transform3D::identity() noexcept {
    Transform3D t;
    t.m[0] = 1.0;
    t.m[5] = 1.0;
    t.m[10] = 1.0;
    t.m[15] = 1.0;
    return t;
}

Point3D Transform3D::apply(Point3D p) const noexcept {
    return Point3D{m[0] * p.x + m[1] * p.y + m[2] * p.z + m[3],
                   m[4] * p.x + m[5] * p.y + m[6] * p.z + m[7],
                   m[8] * p.x + m[9] * p.y + m[10] * p.z + m[11]};
}

Vector3D Transform3D::apply(Vector3D v) const noexcept {
    return Vector3D{m[0] * v.x + m[1] * v.y + m[2] * v.z,
                    m[4] * v.x + m[5] * v.y + m[6] * v.z,
                    m[8] * v.x + m[9] * v.y + m[10] * v.z};
}

Transform3D Transform3D::composed(const Transform3D& o) const noexcept {
    Transform3D r;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            double sum = 0.0;
            for (int k = 0; k < 4; ++k) {
                sum += m[row * 4 + k] * o.m[k * 4 + col];
            }
            r.m[row * 4 + col] = sum;
        }
    }
    return r;
}

std::string to_string(const Transform3D& t) {
    std::string out;
    out.reserve(256);
    out.append("Transform3D{");
    for (int row = 0; row < 4; ++row) {
        out.append("[");
        for (int col = 0; col < 4; ++col) {
            out.append(std::to_string(t.m[row * 4 + col]));
            if (col < 3)
                out.append(", ");
        }
        out.append(row < 3 ? "] " : "]}");
    }
    return out;
}

}  // namespace mycad::domain
