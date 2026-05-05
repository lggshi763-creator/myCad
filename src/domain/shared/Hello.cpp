#include <mycad/domain/Hello.hpp>

#include <string>

namespace mycad::domain {

// Domain layer is constrained to zero external dependencies (ADR-0002).
// Use only the C++ standard library — no fmt, no spdlog, no Qt.
std::string greet(std::string_view name) {
    std::string out;
    out.reserve(name.size() + 9);  // "Hello, " + name + "!"
    out.append("Hello, ");
    out.append(name);
    out.append("!");
    return out;
}

}  // namespace mycad::domain
