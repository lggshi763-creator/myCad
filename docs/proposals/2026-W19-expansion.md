# 2026-W19 — 四个开放问题的分析与设计

> 本文是**分析与设计稿**，非执行任务。每节末尾给出 3-5 条可直接复制粘贴给 AI 执行的提示词。
>
> 待你逐一确认后再下达。
>
> 关联：[docs/devlog/2026-W19.md §5 待办](../devlog/2026-W19.md)

---

## 0. 文档目的与处理流程

| 章节 | 主题 | 输出形态 |
|---|---|---|
| §1 | AI 协作工具链（multi-agent / skill / MCP / rules） | 落地为 `CLAUDE.md` + 3 个 skill + 工作流约定 |
| §2 | 2D 工程图（识别 + 生成）模块 | 1 份 ADR + roadmap 插入 + 模块骨架设计 |
| §3 | CI 选型与自托管路径 | 决策 + 触发条件 + 备选方案文档 |
| §4 | 未决事项的跟踪机制 | 工作流约定 + `docs/inbox.md` 引入 |

**处理流程**：你读完每节 → 选 ✅采纳 / ✏️修订 / ❌否决 → 把对应"可执行提示词"贴回给我 → 我执行。

---

## §1. 如何把 multi-agent / skill / MCP / rules 用进 myCad

### 1.1 现状盘点

**你正在用的**：
- 单 Claude Code 主对话（含 sub-agent 能力但用的少）
- DeepSeek 作为补充编码工具（外部）
- 没有 `CLAUDE.md` / `.cursorrules` 等项目级 AI 配置文件
- 没有自定义 skill
- 没有 MCP 配置

**这周已经无意识使用过**：
- `Explore` sub-agent（让它读 docs §8.3 + ADR-0008） — 高价值
- `Plan` sub-agent（未实际使用）
- `general-purpose` sub-agent（未实际使用）
- 内置 skill（写自定义脚本时可调）

**没用上但应该用**：项目级配置 + 自定义 skill + 跨会话上下文持久化。

### 1.2 概念区分

| 维度 | 作用域 | 触发方式 | 适合场景 |
|---|---|---|---|
| **CLAUDE.md** | 整个 Claude session 自动加载 | 自动 | 项目级**规约 / 不变量 / 永久要求** |
| **Skill** | 命令式，明确触发 | 显式 invoke 或 trigger 词命中 | 高频可复用过程（"修构建"、"加 ADR"） |
| **Sub-agent** | 单次任务 | 主 agent spawn | 大量 token 消耗的探索 / 设计 / 评审 |
| **MCP server** | 外部工具桥 | 主 agent 调 tool | 接 GitHub / Jira / DB / 自研服务 |
| **Hook**（settings.json） | harness 自动 | 触发器（PostToolUse 等） | 不可信 AI 的强制约束（commit 前必跑 lint） |

**关键概念差**：CLAUDE.md 是"被动告知"，skill 是"主动调用"，agent 是"分支思考"，MCP 是"延伸感知"。

### 1.3 为 myCad 量身设计的方案

#### 1.3.1 必做：`CLAUDE.md` at root（最高 ROI 单一动作）

把**本周血泪教训中所有 AI 协作中关键的不变量**写进去。每次 Claude 启动会话自动加载。

骨架建议（要执行时让我写完整版）：

```markdown
# myCad — Claude / AI 协作约束

## 永远遵守
- Domain 层零外部依赖（ADR-0002）。不许用 fmt / std::format / spdlog / Qt。
- 头文件路径：src/<layer>/<module>/include/mycad/<layer>/<X>.hpp。
  include 形式：#include <mycad/<layer>/<X>.hpp>。
- Doxygen 注释：/// + @brief 英文 + 中文详情 + @param/@return/@throws。
  禁用 /** ... */。

## 永不做
- 不用 File → Open → Folder 打开 myCad（必须 File → Open → CMake...）。
- 不在 CMake preset 文件根加任何注释字段（$comment / _comment 都会破坏 schema）。
- 不"为修构建错而加 cache var" — 删掉它会复现错误才算真修复。

## 工具链特殊要求（本机）
- VCPKG_ROOT = E:/dev/vcpkg
- 代理 = 127.0.0.1:7897
- 本机只装 VS 18 → 用 preset `local-debug`。CI 走 `vs2022-x64-debug` 不变。

## 出错时的反应路径
- vcpkg 报"port 不存在" → 先清 %LOCALAPPDATA%\vcpkg\registries\{git,git-trees}
- vcpkg 报 SSL 35 → tools/vcpkg-rescue.ps1
- VS 跑出 out/build/x64-debug → 删 .vs/ 重开
```

效果：DeepSeek / 后续任何 Claude session 启动就**默认知道这些**，不用每次提醒。

#### 1.3.2 必做：3 个 myCad 专用 skill

写在 `.claude/skills/`，每个一个 `.md` 文件 + 触发条件。

| Skill 名 | 触发 | 干什么 |
|---|---|---|
| `vcpkg-rescue` | 用户说 "vcpkg 又挂了 / SSL error 35 / 喂 tarball" | 调 `tools/vcpkg-rescue.ps1`，确认结果 |
| `adr-new <题目>` | 用户说 "写 ADR / 加 ADR" | 找下一个空闲编号、生成模板、塞入 docs/adr/ |
| `mycad-style-check` | 任何 hpp/cpp 改动后 | 跑 clang-format --dry-run + 检查 Doxygen 注释规范（/// 而非 /\*\*\*/、有 @brief、英文 brief 模式） |

#### 1.3.3 应做：sub-agent 用法约定

写进 CLAUDE.md：

- **Explore agent**：所有"读 docs/、搜代码、找类似实现"操作 — 主 agent 不要直接读
- **Plan agent**：Sprint 起始的设计决策（如 Sprint 0.2 值对象设计哲学）
- **general-purpose**：跨多文件的实现任务（如"为所有值对象批量加 Doxygen 注释"）
- **不用 sub-agent 的场景**：单文件改动、user 实时调试、明确流程的执行

#### 1.3.4 暂缓：MCP

当前没有外部系统需要桥接。等 push 解决后**第一个值得加**：`github` MCP（issue / PR 管理直接在 chat 里完成）。

未来候选：
- 自建 vcpkg cache MCP（解决 schannel CRL 这类反复出现的网络问题）
- Jira / Linear MCP（如果团队扩大）

**现阶段不做**。

#### 1.3.5 应做：hook（settings.json）

最小集：

```json
{
  "hooks": {
    "PostToolUse": [{
      "matcher": "Edit|Write",
      "hooks": [{
        "type": "command",
        "command": "powershell -File tools/check-style.ps1 \"$CLAUDE_FILE_PATH\""
      }]
    }]
  }
}
```

效果：AI 写完 .cpp / .hpp 文件马上跑 clang-format check + Doxygen 检查，AI 自己看到结果就会修。比"等 user pre-commit 拦截"早一个回合。

### 1.4 §1 可执行提示词

待确认后**任选**复制给我：

```
[1.A] 帮我在项目根创建 CLAUDE.md，按 docs/proposals/2026-W19-expansion.md §1.3.1 的骨架填充完整内容，所有"约束 / 不做 / 工具链 / 出错路径"四节都要写，并把本周 devlog 里的"决定的"那一节里的 6 条规约也合并进去。
```

```
[1.B] 帮我创建三个 .claude/skills/ 下的 skill：vcpkg-rescue / adr-new / mycad-style-check。每个 skill 都要含 frontmatter（name + description + 触发条件示例）和具体 procedure。参考 docs/proposals/2026-W19-expansion.md §1.3.2。
```

```
[1.C] 在我的 .claude/settings.local.json 里加一个 PostToolUse hook，Edit/Write 后跑 tools/check-style.ps1 自动检查 clang-format + Doxygen 注释规范。同时帮我写 tools/check-style.ps1 本身。
```

```
[1.D] 把 sub-agent 使用约定（Explore / Plan / general-purpose 何时用、何时不用）追加到 CLAUDE.md 最后一节。
```

---

## §2. 2D 工程图模块 — 识别 + 生成

### 2.1 为什么这是个空白

回看 [docs/architecture/02-technical.md](../architecture/02-technical.md) 的层次图，**完全没有 drawing 子领域**。但工程实际中：

- 装配车间、合同评审、ISO 9001 认证现场审查 — 全部要 2D 工程图作为正式文件
- SolidWorks / Creo / Inventor 的"Drawing module"是核心模块，不是附加
- 国内军工 / 中央企业研发 — 90% 的合同交付物形态是 PDF 工程图（不是 STEP）

myCad 如果只能输出 3D 模型不能给 2D 工程图，**等于不能进入工业 B 端市场**。这是个**P0 级缺失**。

### 2.2 两个子模块的差别

| 维度 | Drawing **生成** | Drawing **识别** |
|---|---|---|
| 输入 | 3D 模型 | PDF / 扫描图像 / DXF / DWG |
| 输出 | 多视图工程图（PDF / DXF） | 重建的 3D 模型（或带语义的 2D 草图） |
| 难度 | 中（确定性算法） | 高（需要 ML / VLM） |
| 优先级 | **P0**（合同交付） | **P1-P2**（差异化卖点，AI-native） |
| 何时排入 roadmap | Phase 1.B（Sketch + Feature MVP 之后） | Phase 2 差异化阶段 |

### 2.3 架构落位（与现有四范式的兼容）

#### 2.3.1 Domain 层新增模块 `domain/drawing/`

新聚合根：`Drawing`（一份图纸）。
- 包含：
  - `Sheet`（图框 + 标题栏 + 多个 view）
  - `View`（一个具体投影：正视 / 俯视 / 侧视 / 等轴 / 剖视 / 局部放大）
  - `Dimension`（尺寸标注，引用 3D 模型的几何 ID）
  - `Annotation`（注释、形位公差、表面粗糙度符号）
  - `BOM`（明细栏，自动从装配体抽出）
- **引用** 3D 模型的 AggregateId（不嵌入），通过 ID 弱关联
- 新事件：`SheetCreated` / `ViewProjected` / `DimensionPlaced` / `AnnotationAdded` / `BomGenerated`

#### 2.3.2 Infrastructure 层新增 `infrastructure/drawing/`

| 子模块 | 责任 | 候选库 |
|---|---|---|
| `DxfPort` 实现 | DXF / DWG 读写 | `libdxfrw`（开源、纯 C++）；DWG 用 LibreDWG（GPL，需评估） |
| `PdfRenderer` | 渲染 PDF 输出 | Qt Print Support（已有依赖，免费）；HPDF（备选） |
| `ProjectionEngine` | 3D → 2D 视图投影 + HLR（隐藏线消除） | OCCT 自带 `HLRBRep` 模块（已有依赖，免费） |
| `BomGenerator` | 装配树 → 明细栏 | 自研，纯算法 |

#### 2.3.3 识别模块 `infrastructure/recognition/`

| 子模块 | 责任 | 候选 |
|---|---|---|
| `RasterToVector` | 扫描图像 → 矢量线段 / 弧 / 文本 | OpenCV + Tesseract（OCR） |
| `SemanticParser` | 矢量元素 → 语义（"这是一个直径标注 ⌀10"） | **VLM API**（Claude / GPT-4V）+ 本地后处理 |
| `Sketch3DReconstructor` | 多视图 → 3D 模型 | 自研 + OCCT；高难，Phase 2 后期 |

**架构原则**：识别模块作为 `infrastructure/` 的 adapter，**不污染 domain 层**。识别结果转换为命令（CommandBus）"创建草图 / 添加约束 / 创建特征"由现有 application 层处理。这样识别 / 不识别只是输入侧不同，下游一致。

### 2.4 Roadmap 插入建议

```
Phase 1.A:  Sketch MVP（已规划）
Phase 1.B:  Feature MVP（已规划）
+ Phase 1.C: 单视图 Drawing 生成 MVP   ← 新增
            - 单 sheet, 单 view, 自动正三视图, 简单尺寸标注, PDF/DXF 导出
Phase 1.D:  装配 MVP（已规划）
+ Phase 1.E: BOM + 多视图 Drawing      ← 新增
            - 装配体 → BOM 自动抽取, 多 sheet, 剖视图, 局部放大

Phase 2.A:  AI-native 草图（已规划）
+ Phase 2.B: Drawing 识别 MVP          ← 新增
            - 单一 PDF / 扫描图 → 重建 3D 草图（不含约束求解）
Phase 2.C:  AI 约束补全（已规划）
+ Phase 2.D: 全图纸识别 + 智能约束     ← 新增
            - 多视图融合 + GD&T 标注识别 + 自动 sketch 重建
```

### 2.5 待写的 ADR

| ADR-编号 | 主题 |
|---|---|
| ADR-0010 | Drawing 模块的 Domain 边界与事件设计 |
| ADR-0011 | DXF / DWG 库选型（libdxfrw / LibreDWG / OCCT 内置） |
| ADR-0012 | Drawing 识别走 VLM API 还是本地模型（成本 / 隐私权衡） |

### 2.6 §2 可执行提示词

```
[2.A] 帮我写 ADR-0010：drawing 模块的 domain 边界。包含：聚合根 Drawing 的不变量、与 3D 模型的弱耦合方式（按 AggregateId 引用而非嵌入）、5 个核心事件（SheetCreated / ViewProjected / DimensionPlaced / AnnotationAdded / BomGenerated）的设计、以及为什么是新聚合而不是 3D 模型的子部分。
```

```
[2.B] 把 docs/architecture/06-roadmap.md 修改：在 Phase 1 插入 Phase 1.C（drawing 生成 MVP）和 Phase 1.E（BOM + 多视图 drawing），在 Phase 2 插入 Phase 2.B（drawing 识别 MVP）和 Phase 2.D（全图纸识别）。每个新阶段给出任务清单 / 估时 / 验收标准，参考现有 Phase 1.A 的写作风格。
```

```
[2.C] 帮我写 ADR-0011：DXF / DWG 库选型。对比 libdxfrw（LGPL, C++）/ LibreDWG（GPL, C）/ OCCT 自带 DXF（受 OCCT 协议）。结论倾向 libdxfrw + 不支持 DWG（让用户外部转）。给出引入 vcpkg.json 的具体改动。
```

```
[2.D] 设计 src/domain/drawing/ 目录骨架：列出所有要落地的头文件（每个用 1-2 行说明），但不写实现。包括 Drawing.hpp / Sheet.hpp / View.hpp / Dimension.hpp / IDrawingProjectionPort.hpp 等。给出 src/domain/drawing/CMakeLists.txt 的初版（含 mycad::drawing alias，依赖 mycad::domain）。
```

```
[2.E] 帮我写 ADR-0012：drawing 识别的实现选型。VLM API（Claude vision / GPT-4V）vs 本地模型（YOLO + CRNN OCR）的权衡。考虑成本（按调用计费）、隐私（识别军工图纸是否能上传到第三方？）、准确率、开发节奏。倾向 Phase 2.B MVP 用 VLM API（拼速度），Phase 2.D 升级时再评估本地化路径。
```

---

## §3. CI 选型 — 是否要换 Jenkins / 自托管

### 3.1 当前栈与压力点

| 维度 | 现状 |
|---|---|
| CI provider | GitHub Actions cloud |
| 矩阵 | 3 OS × 2 build type = 6 runner |
| 单 job 时长 | 估计 60-90 min（OCCT + Qt 编译大头） |
| 月度配额（GHA 私库免费层） | 2000 min/月 |
| 推算用量 | 6 runner × 75 min × 假设 30 次/月 = 13500 min — **远超免费层** |
| 解决思路 | 公开仓库（无限免费）/ 自托管 runner / 减少矩阵 / 切付费 |

myCad 是开源项目（LGPL，ADR-0001），**走公开仓库路线 GitHub Actions 完全免费**，这是当前最优解。但即使免费，单 job 时长太长仍然影响开发节奏（每次 push 等 90 min 反馈）。

### 3.2 候选方案对比

| 方案 | 适合 myCad? | 何时切换 |
|---|---|---|
| **GitHub Actions cloud（现状）** | ✅ 当前 | Phase 0-1 默认 |
| **GitHub Actions self-hosted runner** | ✅ 兼容现有 workflow | 单 job > 30 min 影响开发节奏时，加 1 个本机 Win runner |
| **Drone CI / Woodpecker CI** | ⚠️ Linux 强 / Win+Mac 弱 | 不推荐 — Win/Mac 是 myCad 必备 |
| **Buildkite** | ✅ Hybrid 模式 | 团队 > 5 人时可考虑 |
| **Jenkins** | ❌ 单人维护成本超过收益 | 团队 > 10 人 / 多产品线时 |
| **Gitea + Gitea Actions** | ✅ 完全自托管 + GHA 兼容语法 | 国内服务器 / 想完全规避 GitHub | 
| **GitLab CI（自托管 GitLab）** | ⚠️ 要先迁仓库 | 不推荐迁移 |
| **TeamCity** | ⚠️ Java + 配置 UI 重 | 不推荐 |

### 3.3 触发自托管的具体条件

提前定义**触发器**，避免凭直觉换工具：

| 触发条件 | 行动 |
|---|---|
| **单 job > 60 min 且 ≥ 2 周持续** | 加 1 个 self-hosted Win runner，专跑 vcpkg-heavy job |
| **CI 月失败率 > 30% 因网络** | 同上 — 自托管 runner 在 user 自己的代理后面 |
| **协作者 ≥ 3 人** | 切 Buildkite，保留 GHA 做 trigger |
| **被甲方要求"代码不能离开本地网"** | 切 Gitea + Gitea Actions（私有部署） |
| **任何上述都不发生** | 不动，保持 GHA cloud |

### 3.4 与中国 GFW 环境的特殊关系

GitHub Actions 的 cloud runner 在 Microsoft Azure（美 / 欧）— **不被墙**，从你 push 触发到 runner 跑完都不需要代理。**所以"网络不通"不是切自托管的理由**。

但**你 push 到 GitHub 这一步**确实可能撞 TLS 错误（本周 §1.3.1 的 V7 还卡在这）。**万一长期不能解决** push 问题，备选：

1. **Gitee 镜像** — 国内速度快，Gitee 有自己的 Pages 和有限 CI
2. **Cloudflare Pages / Workers** as 中转
3. 自建 Gitea + Gitea Actions（完全自主）

但这些都是"push 长期失败 ≥ 1 月"的兜底，不该提前做。

### 3.5 当前应做的最佳实践（不换工具）

按"复杂度 / 收益"排序：

1. ✅ 已做：`actions/cache` 缓存 vcpkg binary archives
2. ✅ 已做：`fail-fast: false` 让矩阵并行不互相 cancel
3. ✅ 已做：sanitizers 单独 workflow + 周一 cron
4. ✅ 已做：失败时上传 vcpkg buildtree log
5. ⚠️ 未做：vcpkg `x-gha` binary cache provider（GHA 原生，比 actions/cache 解压快 2-3x）
6. ⚠️ 未做：`paths-filter` — docs-only 改动跳过 build 矩阵
7. ⚠️ 未做：多个独立 workflow 拆分（lint 1 min、构建 60 min、e2e 30 min；按 PR 状态分别要求）
8. ⚠️ 未做：dependabot for vcpkg.json baseline 更新 PR
9. ⚠️ 未做：CI 跑 codecov 覆盖率上报（Phase 1 等代码量上来后做）

### 3.6 §3 可执行提示词

```
[3.A] 把 .github/workflows/ci.yml 升级使用 vcpkg 的 x-gha binary cache provider 替代 actions/cache（设置 VCPKG_BINARY_SOURCES=clear;x-gha,readwrite，并按 vcpkg 文档加 ACTIONS_CACHE_URL / ACTIONS_RUNTIME_TOKEN 的 expose 步骤）。保留原 actions/cache 作为 fallback 注释掉以便回滚。
```

```
[3.B] 给 .github/workflows/ci.yml 加 paths-filter：仅 docs/ / **/*.md 改动时跳过整个构建矩阵，只跑 lint job。
```

```
[3.C] 写 docs/operations/ci-strategy.md，把 docs/proposals/2026-W19-expansion.md §3.3 的"何时切换自托管"触发条件正式化为 living doc。包含 GitHub Actions cloud 的免费配额估算、自托管 runner 的搭建步骤（先按 Windows-only Phase 1 计划写，后面扩）。
```

```
[3.D] 在 .github/workflows/ 下加 dependabot.yml，定期为 vcpkg.json 的 builtin-baseline 创建升级 PR（每月一次）。
```

---

## §4. 当前上下文里的"未来代办项"管理

### 4.1 现状盘点：本周确实积累了多种 TODO

按发生位置分类：

| 类型 | 例子（来自本周） | 数量 |
|---|---|---|
| **代码内 TODO** | natvis 注释化的待解开类型、preset 名 vs2022 / 实际 VS18 | ~10 |
| **devlog 尾巴待办** | V1-V8 验证、push TLS、vcpkg 自动清脚本 | ~6 |
| **架构级缺失** | drawing 模块（§2）、AI 工具链（§1） | ~5 |
| **流程未启用** | hooks 没 git config 启用、CLAUDE.md 不存在 | ~3 |
| **环境性遗留** | VS Installer 卸 14.44、push 测试 | ~2 |

总计约 25 项分散在 6 个不同位置。**没有统一入口**。

### 4.2 三层跟踪机制（推荐）

| 层 | 位置 | 用途 | 生命周期 |
|---|---|---|---|
| **L1 即时** | TODO 注释 in source | 代码上下文相关、< 1 sprint 解决 | 与代码一起删 |
| **L2 周内** | `docs/inbox.md` | 跨文件、本周 / 下周内会回头处理 | 周末 promote 到 devlog 或 L3 |
| **L3 跨周** | GitHub Issues（push 解决后） / `docs/devlog/<week>.md §待办` | 跨 sprint 持续追踪 | 关闭时打勾或链 commit |

**晋升路径**：

```
TODO 注释         → 引用同 sprint 任务（保持原地）
                ↘
                  inbox.md → 周末 review → 升 issue 或入下周 devlog
                ↗
ad-hoc 想法
```

**退路径**：

```
inbox.md 项 → 经评估 → "决定不做"，注明原因，转入 retrospectives/decisions-rejected.md
```

### 4.3 inbox.md 的格式建议

```markdown
# Inbox — 未分类待办

> 跨文件、跨会话的临时记事。每周日清空（晋升 issue / devlog / 架构 ADR / 直接关闭）。

## YYYY-MM-DD（场合 / 由谁触发）

- [ ] 待办内容（关键词：vcpkg / preset / docs / etc）
  - 上下文：1-2 行说明为什么有这个项
  - 候选解：可选，如果已经想过

## 2026-05-06（W19 devlog 写作中）

- [ ] **vcpkg-reset-registry.ps1** — 把"清 registries/{git,git-trees}"沉淀成脚本
  - 上下文：本周复发 3 次 unborn-master 状态
  - 候选解：tools/vcpkg-reset-registry.ps1 = `Remove-Item -Recurse -Force "$env:LOCALAPPDATA/vcpkg/registries/git*"`

- [ ] **preset 改名 vs2022 → vs2026** — 历史包袱
  - 上下文：机器升级 VS 18 后 preset 名误导
  - 候选解：批量改 + 同步 CI workflow + devlog 记录变更
  - 风险：CI 需要同步重命名一批文件，先列影响面再改
```

### 4.4 与 AI 协作的衔接（重要）

这是回到 §1 的连接：**AI agent 应该知道你的待办系统在哪，并主动用它**。

具体规约（要写进 CLAUDE.md）：

> 1. AI 完成主要任务后，**主动检查**自己有没有"额外发现 / 顺手记下了又没做"的 followup —— 有就**自己加进 docs/inbox.md**，不要让 user 手记。
> 2. AI 启动 session 时，**Explore 一遍 docs/inbox.md** —— 如果当前任务跟 inbox 里某条相关，主动提示 user "顺便能把这个一起处理吗"。
> 3. AI 修代码遇到 `TODO(...)` 注释时，**评估能否当场解决** —— 能就解决，不能就在 inbox 增项 + 注释里加 issue 链接（如果有）。

这把 inbox 从"易遗忘的纯人工 list"升级为"AI 协作的标准 ritual"。

### 4.5 §4 可执行提示词

```
[4.A] 帮我创建 docs/inbox.md，初版包含 docs/proposals/2026-W19-expansion.md §4.1 列举的 25 项待办全部条目化（按 §4.3 的格式），并写文档头说明用法、每周日 review 规则。
```

```
[4.B] 把"AI 启动会话主动 Explore inbox / 完成任务后主动加项 / 遇到 TODO 注释主动评估"这三条规约写进 CLAUDE.md（如果你也要执行 [1.A] 的话，合并进去；否则先创建 CLAUDE.md 只含这三条）。
```

```
[4.C] 在 src 全树搜索现有 TODO / FIXME / XXX 注释，统一格式为 TODO(@<owner>, <sprint-X.Y or issue-N>): <text>。把缺归属的项追加到 docs/inbox.md，让我决定是否分配。
```

```
[4.D] 在 .githooks/pre-commit 里加一个软 check：如果新增的代码含 TODO 注释但格式不规范（缺 owner / sprint），warning 但不阻塞。逐步形成习惯。
```

---

## §5. 全部可执行提示词速查（17 条）

按依赖顺序整理（前置条件 → 后续可做）：

| 序 | 编号 | 概要 | 前置 |
|---|---|---|---|
| 1 | [4.A] | 创建 inbox.md 初版 | — |
| 2 | [1.A] | 创建 CLAUDE.md 主体 | — |
| 3 | [4.B] | 把 inbox 协作规约并入 CLAUDE.md | [1.A] [4.A] |
| 4 | [1.D] | 把 sub-agent 用法约定加进 CLAUDE.md | [1.A] |
| 5 | [1.B] | 创建 3 个 skill | — |
| 6 | [1.C] | 加 PostToolUse hook + check-style.ps1 | [1.B] 中的 mycad-style-check |
| 7 | [4.C] | 全树扫 TODO 注释，规整 + 入 inbox | [4.A] |
| 8 | [4.D] | pre-commit 加 TODO 格式 warning | [4.C] |
| 9 | [3.A] | CI 切 vcpkg x-gha binary cache | — |
| 10 | [3.B] | CI 加 paths-filter 跳 docs-only build | — |
| 11 | [3.C] | 写 docs/operations/ci-strategy.md | — |
| 12 | [3.D] | 加 dependabot.yml | — |
| 13 | [2.A] | 写 ADR-0010 drawing domain 边界 | — |
| 14 | [2.B] | 修 ROADMAP.md 插入 drawing phases | [2.A] |
| 15 | [2.C] | 写 ADR-0011 DXF/DWG 库选型 | [2.A] |
| 16 | [2.D] | 设计 src/domain/drawing/ 骨架 | [2.A] |
| 17 | [2.E] | 写 ADR-0012 识别走 VLM 还是本地 | [2.B] |

**建议你的下达顺序**（高 ROI 优先）：

```
Day 1 上午：[4.A] [1.A] [4.B] [1.D]   — AI 协作基础设施 30 min 搞定
Day 1 下午：[1.B] [1.C] [4.C] [4.D]   — 工具 + hook，新增几个文件
Day 1 晚 ：[3.A] [3.B] [3.D]           — CI 优化（不动 workflow 大结构）
Day 2 上午：[2.A] [2.C]                — 两个核心 ADR，决策类
Day 2 下午：[2.B] [2.D] [2.E]          — roadmap + 模块骨架 + 识别 ADR
Day 2 晚 ：[3.C]                       — 文档收尾
```

**你也可以全部 ❌ 否决某一节，或者批量改写后再下达**。等你的指令。
