# CONTEXT for Task: <任务名>

> 由 Claude Code 生成于 <日期 + commit SHA>
> 适用任务：[task-NNNN](./TASKS-template.md#task-nnnn)
> 阅读时间预算：DeepSeek 应在 5 分钟内读完

## 1. 任务概要（1 段话）

<这个任务要做什么，从用户视角；不超过 3 句话>

## 2. 你需要知道的接口

### 2.1 必须实现/修改的接口

```cpp
// 来源：src/<path>/<file>.hpp
// 完整粘贴接口定义，不要省略
class XxxHandler {
public:
    [[nodiscard]] std::expected<Result, Error> doSomething(...);
};
```

### 2.2 你将调用的接口（已存在，不要修改）

```cpp
// 来源：src/<path>/<file>.hpp
class IXxx {
    virtual std::expected<X, Error> someMethod() = 0;
};
```

### 2.3 相关值对象

```cpp
struct Point2D { double x, y; };
struct Line2D { Point2D start, end; };
// ... 完整定义
```

## 3. 架构约束（必须遵守）

- **【硬约束】** <例如：此文件位于 Domain 层 → 禁止 #include 任何 OCCT/Qt/OpenGL 头>
- **【硬约束】** <例如：所有错误返回 std::expected，不要 throw>
- **【硬约束】** <例如：业务方法返回 std::vector<DomainEvent>，不要直接修改成员>
- **【软约束】** <例如：优先使用 std::ranges 而非裸循环>
- **【软约束】** <例如：命名遵循：类 PascalCase、方法 camelCase、成员 trailing_underscore_>

## 4. 禁止事项（常见错误模式）

- ❌ 不要使用裸 new / delete（用智能指针或值类型）
- ❌ 不要在 Domain 层调用任何 OCCT API
- ❌ 不要直接 `std::cout` 调试（用 spdlog）
- ❌ 不要 catch (...) 然后 swallow（除非明确意图）
- ❌ 事件类的字段不要加 setter，构造完即不可变
- ❌ <任务特定的禁止事项>

## 5. 代码风格约定

- C++20 标准，启用 `-Wall -Wextra -Wpedantic -Werror`
- 头文件命名 PascalCase.hpp（与类同名）
- 单文件单类（除非紧密关联的小类）
- `#include` 顺序：本类自身 → 项目内 → 第三方 → STL
- 使用 `namespace mycad::<layer>::<module>` 而非 `using namespace`
- Doxygen 注释（`/// ...` 或 `/** ... */`）写在 public 接口上方

## 6. 验收标准

- [ ] 编译通过（warnings = 0）
- [ ] clang-tidy 通过（项目根的 .clang-tidy）
- [ ] clang-format 已应用
- [ ] Visual Studio Test Explorer 中所有用例 green
- [ ] 不破坏现有测试（CI green）
- [ ] 新增 public 方法有 Doxygen 注释

## 7. 测试期望

你必须为新代码编写以下测试（详细见任务卡的"验收用例"段）：

1. <场景 1：正常路径>
2. <场景 2：边界条件>
3. <场景 3：错误路径>
4. ...

测试位置：`tests/<module>/<file>_test.cpp`

测试框架：Catch2 v3

```cpp
TEST_CASE("<被测对象>::<方法> <场景描述>", "[<module>]") {
    // Arrange
    // Act
    // Assert
    REQUIRE(...);
}
```

## 8. 参考实现（可借鉴的类似代码）

- **类似实现 1**：`src/<path>/<file>.cpp` 中 `<className>::<methodName>` 的实现思路
- **类似实现 2**：`src/<path>/<file>.cpp` 中的 `std::expected` 错误处理模式
- **类似测试**：`tests/<path>/<file>_test.cpp` 中的测试结构

## 9. 不在本任务范围内（不要做）

- ❌ 不要修改 `<某接口>` 接口
- ❌ 不要触动 `<某模块>` 实现
- ❌ 不要做 UI 层改动
- ❌ 不要"顺便"重构其他代码（单独开 issue）
- ❌ 不要修改 vcpkg.json 或 CMakeLists.txt（除非任务明确要求）

## 10. Visual Studio 工作环境提示

- 使用 `vs2022-x64-debug` preset（已在 `CMakePresets.json`）
- F5 调试启动 mycad_app；右键测试在 Test Explorer 调试单测
- 保存文件即自动 clang-format
- clang-tidy 警告显示在 Error List

## 11. 完成后的归档

- 任务卡状态改为 `done`
- 把本文件移动到 `archived/<year-quarter>/CONTEXT-task-NNNN.md`
