#include <mycad/domain/Hello.hpp>

#include <string>

namespace mycad::domain {

// ADR-0002: domain layer is zero-dependency. Implement with std::string
// concatenation, NOT fmt::format / std::format / QStringBuilder.
std::string greet(std::string_view name) {
    constexpr std::string_view prefix{"Hello, "};
    constexpr std::string_view suffix{"!"};

    std::string out;
    out.reserve(prefix.size() + name.size() + suffix.size());
    out.append(prefix);
    out.append(name);
    out.append(suffix);
    return out;
}

}  // namespace mycad::domain

int bad_format(){return 0;}
