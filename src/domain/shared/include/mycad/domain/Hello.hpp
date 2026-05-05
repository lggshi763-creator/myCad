#pragma once

#include <string>
#include <string_view>

namespace mycad::domain {

/**
 * @brief Generates a greeting string for the given name.
 *
 * @param name The name to greet. May be empty or contain Unicode characters.
 * @return A greeting string in the format "Hello, <name>!".
 */
std::string greet(std::string_view name);

}  // namespace mycad::domain
