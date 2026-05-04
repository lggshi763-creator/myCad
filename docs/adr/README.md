# ADR — Architecture Decision Records

本目录存放 myCad 项目所有"架构级决策"的可追溯记录。

回到架构总目录：[../../ARCHITECTURE.md](../../ARCHITECTURE.md)

## 什么是 ADR

ADR（Architecture Decision Record）= 一个**轻量、不可变、可追溯**的决策记录单元。

每条 ADR 回答一个问题："**为什么**我们当时选了 X 而不是 Y / Z？"

代码会改、文档会更新，但**决策的历史**应该被保留 — 这样未来回看时能理解"当时的语境"，而不是把已被驳回的方案重新讨论一遍。

## 何时写 ADR

写：
- ✅ 引入新的第三方依赖（库、框架、工具）
- ✅ 改变跨多模块的接口或协议
- ✅ 选择两个方案中的一个，且选择不可逆
- ✅ 修改文件格式或事件 schema
- ✅ 改变项目的协议、构建系统、CI 流程
- ✅ 反转之前的某条 ADR（新 ADR 的 Status: `Supersedes ADR-NNNN`）

不写：
- ❌ 单文件内的实现细节
- ❌ Bug 修复
- ❌ 性能优化（除非引入新算法或新依赖库）
- ❌ 文档变更
- ❌ 命名重构

## 现有 ADR 索引

| 编号 | 标题 | 状态 | 主题 |
|---|---|---|---|
| [ADR-0001](./ADR-0001-license-lgpl-3.md) | 采用 LGPL-3.0-or-later 协议 | Accepted | 协议 |
| [ADR-0002](./ADR-0002-domain-zero-deps.md) | Domain 层零外部依赖 | Accepted | 架构边界 |
| [ADR-0003](./ADR-0003-events-immutable.md) | 领域事件不可变 + 过去时命名 | Accepted | 事件溯源 |
| [ADR-0004](./ADR-0004-occt-as-geometry-kernel.md) | OpenCASCADE 作为几何内核 | Accepted | 第三方依赖 |
| [ADR-0005](./ADR-0005-opengl-not-vulkan.md) | OpenGL 4.5 而非 Vulkan | Accepted | 渲染 |
| [ADR-0006](./ADR-0006-plugin-double-abi.md) | 插件双 ABI 边界（C 入口 + C++ Host） | Accepted | 插件系统 |
| [ADR-0007](./ADR-0007-entt-as-ecs.md) | EnTT 作为 ECS 框架 | Accepted | 第三方依赖 |
| [ADR-0008](./ADR-0008-visual-studio-toolchain.md) | **Visual Studio 2022 作为主 IDE 与构建环境** | Accepted | 工具链 |
| [ADR-0009](./ADR-0009-self-host-roadmap.md) | **第三方依赖隔离与长期自研路线** | Accepted | 长期独立性 |

## 状态流转

```
Proposed ──[review]──► Accepted ──[新决策]──► Superseded by ADR-NNNN
                ▲          │
                │          └──[弃用]──► Deprecated
                └──[拒绝]──► Rejected
```

## 编号规则

- 编号从 `0001` 起，4 位数字
- 一旦分配永不复用，即便 ADR 被 Reject 也保留编号占位
- Reject / Superseded 的 ADR 不删除（历史价值）

## 模板

新写 ADR 时复制 [ADR-template.md](./ADR-template.md) 即可。

## RFC 流程（社区贡献）

非项目维护者提议的重大架构变更走 "ADR-RFC" 流程：

1. 在 GitHub Discussions 的 `architecture` 分类发帖，标题前缀 `[RFC]`
2. 收集 2 周反馈
3. 维护者 Accept 后，提议者把 RFC 整理为 ADR PR
4. 维护者 review + 合并

## 维护责任

- **草稿**：Claude Code 起草占主流（结构稳定）
- **审定**：人工最终决策（Status: Accepted 必须由维护者写）
- **复盘**：每半年扫描一次所有 Accepted ADR，标记是否仍然有效
