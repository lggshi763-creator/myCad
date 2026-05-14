#include <mycad/domain/EventId.hpp>

#include <string>

namespace mycad::domain {

namespace {
// Appends count bytes starting at start as lowercase hex into out.
void appendHex(std::string& out, const std::array<std::uint8_t, 16>& bytes, int start, int count) {
    static constexpr char kHex[] = "0123456789abcdef";
    for (int i = start; i < start + count; ++i) {
        out.push_back(kHex[(bytes[i] >> 4) & 0xF]);
        out.push_back(kHex[bytes[i] & 0xF]);
    }
}
}  // namespace

std::string to_string(EventId id) {
    std::string out;
    out.reserve(36);
    appendHex(out, id.bytes, 0, 4);
    out.push_back('-');
    appendHex(out, id.bytes, 4, 2);
    out.push_back('-');
    appendHex(out, id.bytes, 6, 2);
    out.push_back('-');
    appendHex(out, id.bytes, 8, 2);
    out.push_back('-');
    appendHex(out, id.bytes, 10, 6);
    return out;
}

}  // namespace mycad::domain
