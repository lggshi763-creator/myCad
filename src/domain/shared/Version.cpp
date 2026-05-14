#include <mycad/domain/Version.hpp>

#include <string>

namespace mycad::domain {

std::string to_string(Version v) {
    std::string out;
    out.reserve(24);
    out.push_back('v');
    out.append(std::to_string(v.value));
    return out;
}

}  // namespace mycad::domain
