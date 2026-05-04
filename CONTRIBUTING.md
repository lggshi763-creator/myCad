# 贡献指南

感谢你考虑为 myCad 贡献！本文档说明从 fork 到 PR 合并的完整流程。

> 项目处于 **Phase 0（地基）**，许多接口仍在演进。如果你不确定某个方向是否值得投入，**先开 Discussion 而非 PR**。

## 我可以贡献什么？

| 类型 | 难度 | 推荐 |
|---|---|---|
| 修文档错字 / 翻译改进 | ★ | 任何贡献者 |
| 修 `good first issue` 标签的 bug | ★★ | 新人入门 |
| 写测试（单元 / 集成） | ★★ | 熟悉某模块后 |
| 实现 P1/P2 功能 | ★★★ | 有 CAD / C++ 经验 |
| 性能优化 | ★★★ | 有 profile 经验 |
| 第三方插件 | ★★ | 用 SDK，与主仓低耦合 |
| 架构提议 | ★★★★ | 走 ADR-RFC 流程，见下文 |

## 准备开发环境

### Windows（主推荐）

详见 [docs/architecture/08-vs-toolchain.md](./docs/architecture/08-vs-toolchain.md) 的 §8.2。

简版：
1. 安装 Visual Studio 2022 Community + C++ 桌面工作负载
2. clone vcpkg → 设环境变量 `VCPKG_ROOT`
3. clone myCad → VS Open → CMake → 选 `vs2022-x64-debug` preset
4. F5 调试运行

### Linux

```bash
sudo apt install build-essential cmake ninja-build git pkg-config
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT=~/vcpkg

git clone https://github.com/<owner>/myCad.git
cd myCad
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug
```

### macOS

```bash
brew install cmake ninja git pkg-config
# 同 Linux 流程，preset 改 macos-clang-debug
```

## 开发流程

### 1. 找一个 Issue

- [Good first issue](https://github.com/<owner>/myCad/labels/good%20first%20issue) — 推荐起步
- [Help wanted](https://github.com/<owner>/myCad/labels/help%20wanted) — 维护者明确希望外部贡献
- 自己提一个 Issue 描述想做的事 — **先讨论再开发**，避免做了一半发现方向不对

### 2. Fork + 创建分支

```bash
git checkout -b feat/issue-NNN-short-description
# 或 fix/, docs/, refactor/, test/
```

分支命名规范：`<type>/issue-<number>-<short-name>`，type ∈ {feat, fix, docs, refactor, test, chore, perf}

### 3. 编码

**核心规则**：
- 遵守 [docs/architecture/](./docs/architecture/) 的架构边界（特别是 Domain 零依赖、事件不可变、Tier A 类型隔离）
- 一个 PR 只做一件事，不混合 feature + refactor + 格式化
- 新增 public API 必须有 Doxygen 注释
- 改动可能失败的方法必须返回 `std::expected`，不要 throw

**风格自动化**：
- VS：保存即 clang-format（已在 IDE 配置）
- 命令行：`clang-format -i <files>`
- 提交前：pre-commit hook 自动检查

### 4. 写测试

- **单元测试**：放 `tests/<module>/`，用 Catch2
- 新增 public 方法必须有对应单测
- 修 bug 必须附 regression test
- 覆盖率目标：核心模块 ≥ 80%

```cpp
// 示例：tests/domain/sketch/sketch_test.cpp
#include <catch2/catch_test_macros.hpp>
#include "mycad/domain/sketch/Sketch.hpp"

TEST_CASE("Sketch::addLine produces SketchEntityAdded event", "[sketch]") {
    auto sketch = makeEmptySketch();
    auto events = sketch.addLine({0,0}, {10,0}, opId, userId);
    REQUIRE(events.has_value());
    REQUIRE(events->size() == 1);
    // ... 验证事件类型与字段
}
```

### 5. 提交（Commit）

使用 [Conventional Commits](https://www.conventionalcommits.org/zh-hans/) 规范：

```
<type>(<scope>): <subject>

<body>

<footer>
```

例：

```
feat(sketch): add 圆弧约束 Tangent

实现 Tangent 约束（直线-圆弧 / 圆弧-圆弧）的几何方程与 PlaneGCS 集成。
单测覆盖 5 个典型场景。

Closes #123
```

允许的 type：`feat | fix | docs | refactor | perf | test | chore | build | ci`

### 6. 提 PR

PR 标题用 conventional commits 风格。PR 描述用模板：

```markdown
## What
<改动内容简述>

## Why
<解决什么问题 / Issue 链接>

## How
<实现思路要点>

## Verification
- [ ] 本地构建通过
- [ ] 单测通过 (`ctest`)
- [ ] 无新增 clang-tidy 警告
- [ ] 文档已同步（如果改了公共 API）
- [ ] 影响架构边界 → 已写/更新 ADR

## Related
- Issue: #
- ADR: ADR-NNNN（如适用）
```

### 7. 等待 review

- 一般 1-7 天内首次 review
- 维护者使用 [docs/architecture/03-ai-workflow.md §3.6.3](./docs/architecture/03-ai-workflow.md) 的合规审查模板（Claude Code 辅助）
- 反馈分两类：
  - 🔴 **阻塞性**（必修）：架构违例、API 破坏、关键 bug
  - 🟡 **非阻塞建议**：可在后续 PR 处理

### 8. 合并

- 维护者负责 merge（个人项目阶段）
- 默认 squash merge（保持主线整洁）
- 合并后你的 contributor 名字会出现在 release notes

## 架构提议（ADR-RFC 流程）

如果你想提议**架构级**变更（新增依赖、修改公共接口、改变事件 schema 等）：

1. 在 GitHub Discussions 的 `architecture` 分类发帖，标题前缀 `[RFC]`
2. 描述你的提议（用 [ADR 模板](./docs/adr/ADR-template.md) 即可）
3. 收集 2 周反馈
4. 维护者 Accept 后，提议者把 RFC 整理为 ADR PR
5. 维护者 review + 合并

不要直接提"修改架构的 PR" — 没经讨论的架构变更几乎一定会被拒绝。

## 反馈渠道

| 场景 | 渠道 |
|---|---|
| Bug 报告 | GitHub Issue（用 bug 模板） |
| 功能请求 | GitHub Discussion，类别 `ideas` |
| 架构讨论 | GitHub Discussion，类别 `architecture` |
| 求助 | GitHub Discussion，类别 `q-a` |
| 安全漏洞 | 邮件 security@<domain>（暂用项目维护者邮箱） |

## 行为准则

参与本项目即代表你接受我们的 [Code of Conduct](./CODE_OF_CONDUCT.md)（基于 Contributor Covenant 2.1）。

简而言之：**对人好一点**。

## 协议

提交 PR 即表示你同意你的贡献以 [LGPL-3.0-or-later](./LICENSE) 协议发布。

我们**不**采用单独的 CLA（Contributor License Agreement） — PR 本身即是协议接受。

## 鸣谢

每个 release 的 CHANGELOG 会列出贡献者。第三个被合并的 PR 起，你会被加入 [CONTRIBUTORS.md](./CONTRIBUTORS.md)。

---

> 有疑问？先看 [docs/architecture/](./docs/architecture/)，再发 Discussion。
