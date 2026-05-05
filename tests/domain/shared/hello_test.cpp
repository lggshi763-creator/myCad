#include <mycad/domain/Hello.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>

namespace mycad::domain::tests {

TEST_CASE("greet returns greeting with given name", "[domain][shared]") {
    REQUIRE(greet("World") == "Hello, World!");
}

TEST_CASE("greet handles empty string", "[domain][shared]") {
    REQUIRE(greet("") == "Hello, !");
}

TEST_CASE("greet handles Unicode names", "[domain][shared]") {
    REQUIRE(greet("世界") == "Hello, 世界!");
    REQUIRE(greet("Привет") == "Hello, Привет!");
}

}  // namespace mycad::domain::tests
