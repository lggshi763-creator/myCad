# ADR-0002: Domain 层零外部依赖

- **Status**: Accepted
- **Date**: 2026-05-03
- **Decider**: 项目维护者
- **Author**: Claude Code (drafted) / 人工 (Accepted)

## Context

myCad 采用 DDD 六边形架构。Domain 层（聚合根、领域事件、值对象、Port 接口）是整个系统的"业务真相所在"。

如果 Domain 层引入第三方依赖（OCCT / Qt / OpenGL / EnTT），会产生连锁问题：
- LGPL/许可证传染范围失控
- 单元测试需要第三方运行时（笨重）
- 自研第三方替代时上层全部受影响（[§九](../architecture/09-self-host-strategy.md)）
- AI 协作工具（DeepSeek）实现 Domain 类时容易"顺手"用第三方类型 → 漂移

不决策的代价：边界模糊会导致日积月累的依赖泄漏，到 Phase 2 时已无法清理。

## Decision

我们决定 **Domain 层（`src/domain/**`）零外部依赖**：

允许的依赖：
- C++20 标准库（STL）
- `<expected>`（C++23 polyfill：`tl::expected`）
- Eigen（仅作为"事实标准的数学库"，例外允许）

**不允许**的依赖：
- OCCT 任何头
- Qt 任何头
- OpenGL / GL3W / GLAD
- EnTT
- SQLite
- 任何 GUI / 图形 / 几何 / 数据库库

通过 CMake 强制：Domain target 不允许 `target_link_libraries` 任何上述库。
通过 CI 强制：grep `#include` 检查违规。

## Considered Alternatives

### Option A: Domain 零依赖（推荐）
- 优点：边界清晰，便于自研替代、测试、AI 协作
- 缺点：早期开发需要写一些"翻译"代码（但量很小）

### Option B: Domain 允许"工具型"依赖（spdlog、fmt）
- 优点：开发体验稍好（直接用 fmt 格式化）
- 缺点：边界开始模糊；后续会放宽到"也允许 Eigen 高级特性，也允许 nlohmann/json，也允许..." → 滑坡

### Option C: 不做层次约束
- 优点：写得快
- 缺点：架构崩塌，无可挽回

## Rationale

- **AI 协作的需要**：Claude Code 审查 + DeepSeek 实现的协作中，"Domain 不能依赖 X"是最容易传达和检查的硬约束。模糊规则在 AI 协作下会迅速崩塌。
- **测试的需要**：Domain 单测应能在 1 秒内跑完上千个用例。引入 OCCT 等重型依赖会让单测变成集成测试。
- **自研路径的需要**：Tier A 依赖（详见 [ADR-0009](./ADR-0009-self-host-roadmap.md)）必须可替换，前提是 Domain 不绑死任何具体实现。
- **协议的需要**：Domain 层的纯 myCad 代码 + Adapter 层的"接触第三方"代码，让 LGPL 边界清晰（[ADR-0001](./ADR-0001-license-lgpl-3.md)）。
- **Eigen 例外**：Eigen 是"事实标准的数学库"，几乎所有 C++ 数值代码都用它。不用 Eigen 而自实现矩阵 = 重新发明轮子。例外不影响其他规则。

## Consequences

### Positive
- 架构边界清晰，AI 协作时硬约束易表达
- Domain 单测极快（毫秒级）
- 自研 Tier A 依赖时上层零影响
- LGPL 边界自然
- 新人理解架构容易

### Negative
- 早期需要写一些"业务对象→几何 Adapter→OCCT"的翻译代码
- 部分场景下需要把第三方类型的能力"重新声明"为 Domain 接口（如 `IGeometryPort` 重新声明 OCCT 能力）

### Neutral
- CMake 中 Domain target 配置严格隔离
- CI 加 grep 检查
- Code review 检查列表加入"是否泄漏第三方类型到 Domain"
- CONTRIBUTING.md 明确说明此约束

## Implementation Notes

### CMake 配置

```cmake
add_library(mycad_domain STATIC
    src/domain/.../Sketch.cpp
    src/domain/.../FeatureTree.cpp
    # ...
)

target_include_directories(mycad_domain PUBLIC
    ${CMAKE_SOURCE_DIR}/src/domain/include
)

# 仅允许这些链接
target_link_libraries(mycad_domain
    PUBLIC
        Eigen3::Eigen        # 例外
    PRIVATE
        # 不允许任何第三方
)

target_compile_features(mycad_domain PUBLIC cxx_std_20)
```

### CI 检查

```yaml
- name: Domain layer zero-dependency check
  run: |
    bad=$(grep -r "#include" --include="*.hpp" --include="*.cpp" src/domain/ | \
          grep -E "(OpenCASCADE|TopoDS_|BRep|Geom_|gp_|QObject|Qt|GL/|gl3w|entt/|sqlite3|spdlog|fmt::)" | \
          grep -v -E "// allowed: ")
    if [ -n "$bad" ]; then
      echo "Forbidden dependencies in Domain layer:"
      echo "$bad"
      exit 1
    fi
```

### CONTRIBUTING.md 说明

明确告知贡献者：
> Domain 层（`src/domain/`）禁止 #include 任何 OCCT / Qt / OpenGL / EnTT / SQLite / spdlog / fmt 头。
> 如果你需要这些库的功能，请：
> 1. 在 Domain 层定义抽象接口（`IXxx`）
> 2. 在 Infrastructure 层实现 Adapter
> 3. 通过依赖注入使用

## References

- [§二 §2.1 整体架构融合哲学](../architecture/02-technical.md)
- [§九 §9.1.2 五层防火墙](../architecture/09-self-host-strategy.md)
- 相关 ADR: [ADR-0001](./ADR-0001-license-lgpl-3.md), [ADR-0009](./ADR-0009-self-host-roadmap.md)
