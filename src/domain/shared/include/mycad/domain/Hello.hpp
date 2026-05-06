#pragma once

#include <string>
#include <string_view>

/// @file
/// @brief Hello-world greeting helper for the domain layer.
///
/// myCad 首个对外接口，作为 Sprint 0.1 的最小可链接示例存在。
/// 它同时兼任两个角色：
///   1. 验证 CMake + vcpkg + Catch2 链路 (tests/domain/shared/hello_test.cpp).
///   2. 演示项目的 Doxygen 注释规约（`///` + `@brief` + 中英双语详情）。

namespace mycad::domain {

/// @brief Builds a greeting string for the given name.
///
/// 给定名字生成形如 `Hello, <name>!` 的问候字符串。Unicode 安全，
/// 接受空字符串（返回 `Hello, !`）。
///
/// 该函数是 domain 层的最小化示例：**仅** 使用 `<string>` 与
/// `<string_view>`，不依赖 fmt、spdlog、Qt 等任何外部库 —— 严格
/// 遵循 ADR-0002（domain 零外部依赖）。
///
/// @param  name  被问候者的名字。允许为空，允许包含任意 UTF-8 字符。
/// @return 拼接好的问候字符串，形如 `"Hello, <name>!"`。
/// @throws std::bad_alloc 字符串内存分配失败时。
/// @thread-safe
std::string greet(std::string_view name);

}  // namespace mycad::domain
