# Sprint 0.1 复盘 — 项目脚手架

**周期**：2026-05-04 ~ 2026-05-13（实际含 Day 1 收尾，约 8 个工作日，累计编码 ~15h）

---

## 计划 vs 实际

| 任务 | 计划估时 | 实际耗时 | 状态 | 偏差原因 |
|---|---|---|---|---|
| CMake + vcpkg 项目结构 | 1d | 1d | ✅ done | 符合 |
| CMakePresets.json（VS / Linux / macOS） | 1d | 2d | ✅ done | VS 18 preset 错位 + toolset 版本锁 |
| tools/vs2022.vsconfig | 0.5d | 0.5d | ✅ done | 符合 |
| GitHub Actions 三平台 CI | 2d | 3.5d | ✅ done | vcpkg baseline 两次失败 + disk full + macOS queue |
| .clang-format / .clang-tidy / git hooks | 1d | 1.5d | ✅ done（VS 18 路径修复留到 Sprint 0.2 Day 1） | hook 未覆盖 VS 18 路径 |
| Catch2 + GoogleBenchmark + Hello World | 1d | 1d | ✅ done | 符合 |
| Doxygen + Sphinx 文档骨架 | 1d | 1d | ✅ done | 符合 |
| tools/visualizers/mycad.natvis 骨架 | 0.5d | 0.5d | ✅ done | 符合 |
| ADR-0001 ~ 0009 | 1d | 1d | ✅ done | 符合 |
| CONTEXT-template + TASKS-template | 0.5d | 0.5d | ✅ done | 符合（还多交付了 prompt-library.md） |

**整体偏差**：+40%（计划 9.5d，实际 ~13d，含 2d 网络/工具链）

---

## 关键产出

### 代码

- 新增文件：~60 个（CMake / CI / hooks / 测试 / 文档 / 工具脚本）
- 核心新增：`src/domain/shared/include/mycad/domain/Hello.hpp` + `Hello.cpp`（domain 零依赖样板）
- 测试：3 个 Catch2 case（hello_test.cpp）+ 1 个 benchmark（hello_bench.cpp）
- 工具：`tools/vcpkg-rescue.ps1`（网络急救）、`tools/vcpkg-reset-registry.ps1`（registry 清理，Sprint 0.2 Day 1 补交）

### 文档

- 新增 ADR：ADR-0001 ~ 0009（9 份不可逆决策）+ ADR-0013（值对象哲学，Sprint 0.2 启动前定稿）
- 新增架构文档：`docs/architecture/` 01-09 全套（~9 章）
- 新增 AI 协作文档：`CONTEXT-template.md` + `TASKS-template.md` + `prompt-library.md`（5 个复用 prompt）
- 新增 `PLAYBOOK.md`（执行哲学，含 §1-10 Sprint 哲学 + 心理健康自检）

### 工具链

- CI：windows-2022 + ubuntu-22.04 × Debug + RelWithDebInfo = 4 个 job（macOS 暂 park）
- Sanitizers：Linux ASan + UBSan 独立 workflow
- 新依赖（vcpkg.json）：14 个 port（opencascade / entt / catch2 / benchmark / eigen3 / tl-expected 等）

---

## 卡点与解决方案

### 1. vcpkg registry unborn-master（复发 3 次）

- **出现日期**：2026-05-04 ~ 05-08，每次网络中断后
- **卡住时长**：每次约 30-60 分钟
- **根因**：网络中断时 packfile 写盘但 ref 没更新，导致 `registries/git` 进入 unborn-master 状态，所有 port 报"不存在"
- **解决方案**：`Remove-Item -Recurse $LOCALAPPDATA/vcpkg/registries/git*` + kill TGitCache
- **后续避免**：沉淀为 `tools/vcpkg-reset-registry.ps1`；第一反应清这两个目录

### 2. VS 18 + preset 写 VS 2022 导致 toolset 错位

- **出现日期**：2026-05-05
- **卡住时长**：~2 小时（链接错误 `__std_find_*_pos_*` 难以定位）
- **根因**：CMake 找不到 VS 17（2022），静默 fallback 到 VS 18 但选了旧 toolset 14.44；vcpkg 独立选了最新 14.50 → CRT 不匹配
- **解决方案**：`CMakeUserPresets.json` 显式 pin `toolset: host=x64,version=14.50` + `generator: Visual Studio 18 2026`
- **后续避免**：新机器第一件事确认 VS 版本 + 检查 CMakeUserPresets.json 里的 generator 字段；`vs2022-*` 前缀是历史包袱，留待统一重构

### 3. schannel CRL 死锁（Windows + GFW 常态）

- **出现日期**：整个 Sprint 期间反复出现
- **卡住时长**：每次 15-30 分钟
- **根因**：CA CRL CDN 被墙 → schannel 握手失败 → `curl error 35`
- **解决方案**：`--ssl-no-revoke` + 代理 7897 + `tools/vcpkg-rescue.ps1`
- **后续避免**：已成共识，不需要每次重新推导；vcpkg 内置 curl 无法直接传 `--ssl-no-revoke`，只能绕行

### 4. git hooks 未覆盖 VS 18 路径

- **出现日期**：2026-05-13（Sprint 0.2 Day 1 发现）
- **卡住时长**：10 分钟
- **根因**：hook 硬编码 `\2022\` 路径，VS 18 装在 `\18\`
- **解决方案**：`.githooks/pre-commit` 增加 VS 18 Community/Professional/Enterprise 三条 fallback
- **后续避免**：hook 里应枚举近几个 VS 年份版本，或改用 `vswhere.exe` 动态查询（Phase 1 可改进）

### 5. AI 生成结果需要 review（DeepSeek 5 处偏离）

- **出现日期**：2026-05-06
- **卡住时长**：30 分钟 review + 修复
- **根因**：DeepSeek 生成了 CMake placebo（`CMAKE_MSVC_RUNTIME_LIBRARY`）、错误 include 路径、封装破坏的 `target_include_directories`、使用了 `<format>`（ADR-0002 违规）
- **解决方案**：逐一 review 并修正
- **后续避免**：AI 给构建改动必须问"删掉这条错误会不会回来"；domain 层 PR 强制 grep `<format>` / `fmt` 检查

---

## 估时偏差分析

- **整体偏差**：+40%（实际 vs 计划）
- **高偏差任务**：
  - CI 配置（原 2d → 实际 3.5d）：根因是 vcpkg baseline 两次失败 + Linux disk full + macOS runner queue；下次估 CI 任务时 × 1.5
  - CMakePresets（原 1d → 实际 2d）：VS 版本错位定位困难；新机器先确认 VS 版本
- **低偏差任务（成功模式）**：
  - Doxygen + ADR 写作类任务全部准时 → 文档类估时准确，继续
  - Catch2 接入 1d 准时 → 测试框架接入估时可信

---

## 给 Sprint 0.2 的调整

- **新增检查**：每个 AI 生成的 PR 必须过 `grep -r "#include" src/domain/` 无 OCCT/Qt/fmt 的门控
- **调整估时**：含 CI 改动的任务一律 × 1.5；纯文档/接口任务估时不变
- **流程改进**：Sprint 启动前先确认 hook 能真正拦截（`clang-format --version` 可用），不只是"文件就位"
- **删除**：不再花时间讨论 preset 改名（语义债，留到集中清理）
- **新增（本 Sprint 遗留）**：`tools/vcpkg-reset-registry.ps1` 已在 Sprint 0.2 Day 1 补交

---

## 心理健康自检（参考 PLAYBOOK §8）

- **工作节奏**：Sprint 期间基本维持 ≤ 3h/day；网络作战日偶尔超时（~4h），但属一次性
- **红色信号检查**：
  - 连续无 commit > 3 天：**否**（最长间隔 2 天）
  - Sprint 估时全面超出：**是**（+40%）→ 根因已分析，主要是工具链不可控；下次 CI 任务加 buffer
  - "嫉妒团队"念头出现：**否**
  - 把 P1 任务降级自我安慰：**否**
- **整体感受**：7/10。工具链摩擦比预期大，但每个卡点都沉淀成脚本或文档，总体有掌控感。

---

## 长期主权指数

- 本 Sprint 引入新依赖：**是**，14 个 vcpkg port（全部 Tier A 外部依赖，有 ADR 覆盖）
- Tier A 泄漏检查：**否**（domain/shared/Hello.hpp 零外部依赖，CI 未加自动检查，Sprint 0.3 补）
- Adapter 测试覆盖率：N/A（Adapter 层 Sprint 0.3 才开始）

---

## 待跟进项（Open Loops）

- [ ] macOS CI（macos-13 runner queue 严重）→ Sprint 0.2 结束后评估是否启用
- [ ] VS 14.44 toolset 卸载（非紧急，当前 preset pin 已兜底）
- [ ] preset 改名 `vs2022-*` → 实际版本（大 churn，集中清理时做）
- [ ] hook 用 `vswhere.exe` 动态查 clang-format 路径（当前硬编码，Phase 1 可改进）

---

## 公开传播素材

- **"vcpkg + GFW 作战全记录"**：schannel CRL / unborn-master / proxy 配置三连 → 适合写成博客或 B 站视频，目标受众：国内 Windows C++ 开发者
- **"AI 协作 review 的 5 个坑"**：DeepSeek 生成的 CMake placebo / 封装破坏 / ADR 违规 → 适合写成短推文或知乎回答

---

> **写作日期**：2026-05-13
> **作者**：lggshi + Claude Code（辅助生成）
