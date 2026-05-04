# ADR-0007: 采用 EnTT 作为 ECS 框架

- **Status**: Accepted
- **Date**: 2026-05-03
- **Decider**: 项目维护者
- **Author**: Claude Code (drafted) / 人工 (Accepted)

## Context

myCad 的技术架构定位"DDD × 事件溯源 × ECS × 微内核"四合一，其中 ECS 用于读模型 / 几何对象管理 / 渲染优化（详见 [§二 §2.4](../architecture/02-technical.md)）。

ECS 是性能关键路径 — 一个典型装配体可能涉及 10⁵ 级别的几何实体，每帧需要遍历、过滤、变换、上传 GPU。

我们必须在"使用现成 ECS 库 vs 自研"之间做选择。

不决策的代价：架构骨架阶段（Phase 0）就需要 ECS 接口落地，否则下游 EventConsumerSystem / RenderSystem 都无法启动。

## Decision

我们决定**采用 EnTT 3.13+ 作为 ECS 后端实现**，并**在 `IEntityRegistry` 接口层封装**，避免 EnTT 类型直接渗入领域层。

为长期独立性，预留自研路径（详见 [§九 §9.2.3](../architecture/09-self-host-strategy.md)）— 自研 ECS 估时仅 3-6 个月，是 Tier A 依赖中**最现实的自研替代候选**。

## Considered Alternatives

### Option A: EnTT (推荐)
- C++ 圈最广泛采用的 ECS 库（GitHub 9k+ stars）
- 头文件库（header-only），集成成本极低
- 性能 SOTA：sparse_set 实现，view/group 优化好
- API 丰富：observer / signal / dispatcher 内置
- MIT 协议，与 LGPL 兼容

### Option B: flecs
- C 语言核心 + C++ 包装
- 内置查询语言，复杂查询表达力强
- 性能与 EnTT 接近
- MIT 协议
- 但 myCad 不需要复杂关系查询

### Option C: 自研
- 完全可控，可裁剪到只支持我们需要的特性
- 不引入外部依赖，避免单一维护者风险
- 但工程量 1-2 个月，且大概率性能不如 EnTT

### Option D: bevy_ecs (Rust) + 跨语言绑定
- bevy_ecs 是公认的现代 ECS 设计典范
- 但跨语言绑定复杂度极高，对单人项目不现实

## Rationale

- **核心约束是时间**：个人开发者的时间预算下，自研 ECS 等于让其他模块停摆 1-2 个月
- **EnTT 的成熟度**：被 Square Enix、Mojang、Unity 等大型项目使用过，稳定性可信
- **API 风格契合 C++20**：EnTT 现代化（concepts、ranges-friendly），不会拖累代码风格
- **flecs vs EnTT 选 EnTT**：flecs 的查询语言虽然强大但 myCad 不需要那么复杂的关系查询；EnTT 更"贴近 C++"
- **未来可替换性**：通过 `IEntityRegistry` 接口封装，万一 EnTT 维护断档可在 1-2 周内切换到 flecs 或自研

## Consequences

### Positive
- Phase 0 ECS 骨架可在 1 周内就绪（vs 自研的 1-2 月）
- 享受 EnTT 的性能优化成果（sparse_set、view 优化）
- 内置 observer 模式可直接用于"Component 变化通知"
- 大量社区文档与示例可参考

### Negative
- 引入外部依赖（vcpkg 管理）
- EnTT 单一维护者风险（缓解：通过 `IEntityRegistry` 封装，必要时可换；详见 [§九 §9.2.3](../architecture/09-self-host-strategy.md)）
- EnTT 编译时间偏长（缓解：开启 ccache + 预编译头）
- 学习曲线（缓解：核心 API 一周可掌握）

### Neutral
- vcpkg.json 增加 entt 依赖
- CMake 配置增加 find_package(EnTT)
- CONTRIBUTING.md 需说明 EnTT 使用约定

## Implementation Notes

### 集成位置

- 接口定义：`src/domain/shared/include/mycad/domain/IEntityRegistry.hpp`
- 实现：`src/infrastructure/ecs/EnttRegistry.{hpp,cpp}`
- 单测：`tests/infrastructure/ecs/`

### 关键约束

EnTT 头**只能**在 `src/infrastructure/ecs/` 内 `#include`，违反将由 CI lint 检测：

```yaml
- name: Verify EnTT type isolation
  run: |
    bad=$(grep -r "#include\s*<entt/" --include="*.hpp" --include="*.cpp" src/ | \
          grep -v "^src/infrastructure/ecs/")
    if [ -n "$bad" ]; then
      echo "EnTT headers leaked outside src/infrastructure/ecs/:"
      echo "$bad"
      exit 1
    fi
```

### 版本锁定

vcpkg.json：

```json
{
  "dependencies": [
    "entt"
  ],
  "builtin-baseline": "<git-sha>",
  "overrides": [
    { "name": "entt", "version": "3.13.2" }
  ]
}
```

### 接口设计

详见 [§五 §5.6](../architecture/05-code-skeletons.md)。

关键点：
- 模板方法（`add` / `get` / `view`）→ Domain 代码用类型化 API
- 类型擦除底层（`addImpl` 等）→ EnTT 实现可用 `entt::registry` 干活
- `View::each` 函数式遍历 → 比迭代器更易优化

### 自研替代触发条件（详见 [§九 §9.2.3](../architecture/09-self-host-strategy.md)）

- EnTT 主仓 12+ 个月无更新
- 出现严重 bug 上游不修
- 协议变化
- 性能瓶颈无法通过现有 API 突破

## References

- [EnTT 官方仓库](https://github.com/skypjack/entt)
- [EnTT 性能 benchmark](https://github.com/abeimler/ecs_benchmark)
- [§二 §2.4 ECS 几何实体系统](../architecture/02-technical.md)
- [§五 §5.6 IEntityRegistry 骨架](../architecture/05-code-skeletons.md)
- [§九 §9.2.3 EnTT 自研路径](../architecture/09-self-host-strategy.md)
- 相关 ADR: [ADR-0002](./ADR-0002-domain-zero-deps.md)（Domain 零依赖巩固隔离）, [ADR-0009](./ADR-0009-self-host-roadmap.md)
