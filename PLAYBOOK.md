# myCad 执行手册（PLAYBOOK）

> 本文件是"从架构方案到第一行代码"的执行指南。
>
> 架构是 What 与 Why；本手册是 **How — 今天就动手做什么**。
>
> **想生成后续 Sprint / Phase 的执行手册？** → [docs/ai-context/prompt-library.md](./docs/ai-context/prompt-library.md)（5 个可复制粘贴的 AI prompt：Sprint 手册 / Phase 概览 / 任务卡 / Sprint 复盘 / 月度风险扫描）

---

## 0. 你现在在哪

✅ 完成：架构方案 9 章 + 9 个 ADR + AI 协作模板
🚧 下一步：**Phase 0 Sprint 0.1 — 项目脚手架（Week 1-2）**

完整路线图见 [ROADMAP.md](./ROADMAP.md) 与 [docs/architecture/06-roadmap.md](./docs/architecture/06-roadmap.md)。

---

## 1. 工作哲学（5 条铁律，违反必死）

### 1.1 时间盒优先于完成度
- **每天**最多编码 2-3 小时（晚上 21:00-23:00 是最佳时段）
- **每周**强制 1 天零编码（推荐周三 OR 周日）
- **每 8 周**休 1 周（"安息周"，不开仓库）
- 估时偏差 50%+ 是正常 — 不要砍质量赶时间

### 1.2 严格按 Phase 顺序
- Phase 0 没完成 → 不要碰 Phase 1
- Sprint 0.3 没完成 → 不要碰 Sprint 0.4
- 不存在"我先做后面那个再回来"

### 1.3 每日提交 commit
- 哪怕只改了一行也要提交
- "活着的项目"对自己和对社区都至关重要
- commit message 用 Conventional Commits（详见 [CONTRIBUTING.md](./CONTRIBUTING.md)）

### 1.4 AI 是同事不是工具
- Claude Code 是架构师 — 找他设计、审查、写文档
- DeepSeek 是工程师 — 找他实现、写测试
- Copilot 是打字员 — 行内补全
- **永远不让 AI 做你不理解的决策**：架构选型、业务方向必须人工拍板

### 1.5 documentation ≥ code
- 任何接口必须有 Doxygen 注释（Claude/DeepSeek 帮写）
- 任何不可逆决策必须写 ADR（Claude 起草）
- 文档过期 = 比没文档更糟

---

## 2. Day 1 — 工作站设置（4-6 小时一次性）

### 2.1 安装 Visual Studio 2022 Community

1. 下载：<https://visualstudio.microsoft.com/zh-hans/vs/community/>
2. 安装时勾选工作负载（详见 [§八 §8.2.1](./docs/architecture/08-vs-toolchain.md)）：
   - ✅ 使用 C++ 的桌面开发
   - ✅ 使用 C++ 的 Linux 和嵌入式开发（用于 WSL 验证）
   - ✅ Git for Windows
3. 单独组件勾选：MSVC v143、Windows 11 SDK、CMake tools、Test Adapter for Catch2、C++ Clang 编译器、C++ Clang-cl

### 2.2 安装 vcpkg

```powershell
# 推荐位置 C:\dev\vcpkg
mkdir C:\dev
git clone https://github.com/microsoft/vcpkg.git C:\dev\vcpkg
C:\dev\vcpkg\bootstrap-vcpkg.bat
C:\dev\vcpkg\vcpkg integrate install
[Environment]::SetEnvironmentVariable('VCPKG_ROOT', 'C:\dev\vcpkg', 'User')
```

重启 VS / PowerShell 让环境变量生效。

### 2.3 装 VS 扩展（仅 4 个）

VS → Extensions → Manage Extensions → Online，搜索安装：
- Test Adapter for Catch2
- Markdown Editor v2
- GitHub Copilot（可选，付费 / 学生 / 开源项目可申请免费）
- Clang Power Tools（可选）

> ⚠️ **不要装** Qt Visual Studio Tools — 项目用纯 CMake AUTOMOC，详见 [§8.2.5](./docs/architecture/08-vs-toolchain.md)。

### 2.4 GitHub 仓库设置

1. 在 GitHub 创建空仓库 `<your-username>/myCad`（建议先设私有，Phase 0 末再开源）
2. 本地把现有项目推上去：
   ```powershell
   cd E:\myCad
   git add .
   git commit -m "docs: initialize architecture and project foundation"
   git remote add origin git@github.com:<you>/myCad.git
   git branch -M main
   git push -u origin main
   ```
3. 在仓库 Settings → Branches 设 main 为 protected branch（要求 PR + status check）

### 2.5 AI 工具账号

- **Claude Code**：<https://claude.com/claude-code>
- **DeepSeek**：<https://chat.deepseek.com>（或 API：<https://platform.deepseek.com>）
- 记录 API key 到密码管理器（不要写到代码里）

### 2.6 验证清单

完成 Day 1 时应能：
- [ ] VS Open → CMake → 选择任意 CMakeLists.txt 不报错（用 vcpkg 自带的 hello 项目验证）
- [ ] PowerShell 中 `echo $env:VCPKG_ROOT` 输出 `C:\dev\vcpkg`
- [ ] `git push` 到 GitHub 成功
- [ ] Claude Code / DeepSeek 都能登录并问个简单问题

---

## 3. 第一周 — Sprint 0.1（脚手架）

> 目标：完成可构建、可测试、有 CI 的空仓库。结束时 `cmake --preset vs2022-x64-debug && cmake --build` 一次成功。

### Day 2（约 2-3h）：CMake + vcpkg 项目骨架

**用 Claude Code，给它如下 prompt**：

```
我在 E:\myCad 项目根目录。已有 docs/、aaa.txt、ARCHITECTURE.md 等。
现在需要落地 Phase 0 Sprint 0.1 的第一项任务：
"创建 CMake + vcpkg manifest 项目结构"

请基于以下约束生成文件：
- C++20 标准
- 主推 Visual Studio 2022（vs2022-x64-debug/release/asan/ubsan 4 个 preset）
- 跨平台支持（Linux GCC、macOS Clang）
- vcpkg manifest 模式
- 依赖：opencascade、qtbase（gui/widgets/opengl/openglwidgets）、qttools、entt、
  eigen3、spdlog、fmt、flatbuffers、catch2、benchmark、tl-expected、
  nlohmann-json、tomlplusplus、zstd、sqlite3
- 顶层 CMakeLists.txt 只做工程级配置，子目录 CMakeLists.txt 各自管理
- 目录结构：src/{domain,application,infrastructure,plugin,ui}, tests/, tools/

请输出：
1. /CMakeLists.txt
2. /CMakePresets.json
3. /CMakeUserPresets.json.template（git ignored 的个人覆盖模板）
4. /vcpkg.json
5. /vcpkg-configuration.json（如果需要）
6. /.gitignore
7. /.gitattributes
8. 每个 src/* 子目录的占位 CMakeLists.txt（仅 add_library 占位即可）

约束遵循 docs/architecture/08-vs-toolchain.md §8.3 与 ADR-0008。
```

**Claude 输出后**：
- 仔细 review 每个文件（不懂的字段问清楚）
- VS Open → CMake → 选 `vs2022-x64-debug` → 应能配置通过（即使没源码）
- `git add . && git commit -m "build: cmake + vcpkg scaffold"`

**验收**：
- [ ] `cmake --preset vs2022-x64-debug` 成功
- [ ] vcpkg 开始下载依赖（不需要等完成 — 依赖编译要 30 min-2h）

### Day 3（约 2-3h）：CI + 代码风格

**用 Claude Code 提示**：

```
基于刚提交的 CMake 骨架，请生成：
1. /.github/workflows/ci.yml — GitHub Actions
   - 矩阵：{ windows-2022, ubuntu-22.04, macos-13 } × { Debug, RelWithDebInfo }
   - 步骤：checkout → 装 vcpkg → cmake configure → cmake build → ctest
   - 用 actions/cache 缓存 vcpkg binary cache
   - Windows 用 vs2022-x64-* preset；Linux 用 linux-gcc-*；macOS 用 macos-clang-*
2. /.github/workflows/sanitizers.yml — 单独的 ASan + UBSan 任务（仅 Linux）
3. /.clang-format — LLVM 基础 + 我们的定制（详见 docs/architecture/08-vs-toolchain.md §8.6）
4. /.clang-tidy — 启用 bugprone-* / cppcoreguidelines-* / modernize-* / performance-*
5. /.editorconfig — 4 空格、UTF-8、LF
6. /.githooks/pre-commit — 用 powershell + bash 兼容，跑 clang-format check
7. /.githooks/commit-msg — Conventional Commits 校验
```

**验收**：
- [ ] push 后 GitHub Actions 跑起来（首次可能因为 vcpkg 编译慢而超时 — 等 binary cache 建立后会快）
- [ ] 故意写一段不规范代码 → pre-commit 阻断
- [ ] 故意写一个不规范 commit message → commit-msg 阻断

### Day 4（约 2h）：测试框架 + 第一个 hello-world

**用 DeepSeek**（任务清晰，适合 DeepSeek 出代码）：

```
基于 myCad 的 CMake + vcpkg 项目（Catch2 v3 + GoogleBenchmark 已在 vcpkg.json）：

1. 在 src/domain/shared/ 创建 Hello.hpp + Hello.cpp，实现一个：
   namespace mycad::domain { std::string greet(std::string_view name); }

2. 在 tests/domain/shared/ 创建 hello_test.cpp，用 Catch2 v3 测试 greet
   测试覆盖：正常名字、空字符串、Unicode 名字

3. 在 benchmarks/ 创建 hello_bench.cpp，用 GoogleBenchmark 跑 greet 性能

4. 更新对应 CMakeLists.txt:
   - src/domain/shared/CMakeLists.txt: add_library(mycad_domain ...)
   - tests/CMakeLists.txt: catch_discover_tests
   - benchmarks/CMakeLists.txt: 仅当 MYCAD_ENABLE_BENCHMARKS 开启

代码风格：
- C++20，PascalCase 类、camelCase 方法、成员尾下划线
- include 顺序：本类 → 项目内 → 第三方 → STL
- 头文件用 #pragma once
- 所有 public 方法有 Doxygen 注释
```

**验收**：
- [ ] VS Test Explorer 中显示 3 个 Catch2 测试，全部 green
- [ ] CI 三平台 green
- [ ] 第一个 commit 通过 pre-commit hook

### Day 5（约 2h）：Doxygen 文档骨架

**用 Claude Code**：

```
为 myCad 设置 Doxygen 文档生成：

1. /Doxyfile — 配置：输入 src/、排除 build/、生成 HTML + XML（XML 给 Sphinx 用）
2. /docs/api-reference/conf.py — Sphinx 配置 + Breathe（桥接 Doxygen XML）
3. /docs/api-reference/index.rst — Sphinx 入口
4. /CMakeLists.txt 增加 add_custom_target(docs)
5. README 增加"如何生成本地文档"段落

我的 Doxygen 风格偏好：
- 用 /// 而非 /** ... */
- @param / @return / @throws / @see
- 中文 + 英文双语（结构化 brief 用英文，详细描述用中文）
```

**验收**：
- [ ] `cmake --build --target docs` 生成 HTML
- [ ] 浏览器打开能看到 greet 函数的文档

### Day 6-7（约 4h）：ADR 落地 + dev log 启动

#### Day 6
- 浏览所有 9 个 ADR（[docs/adr/](./docs/adr/)），review 是否仍然认可
- 不认可的 ADR：标记 `Status: Deprecated` 并说明，**不要直接删**
- 新增 ADR-0010 起的"演进 ADR"（如果 Sprint 0.1 中你做了新决策）

#### Day 7
- 创建 `docs/devlog/2026-WW.md`（按周编号）
- 每天结束时写 5-10 行：今天做了什么、卡在哪、明天计划
- 这是给未来自己看的最重要文档（也是社区博客的素材库）

**Sprint 0.1 验收**：
- [ ] CI 三平台 green
- [ ] 第一个 Catch2 测试在 VS Test Explorer 里 green
- [ ] Doxygen 能生成 HTML
- [ ] 9 个 ADR 全部 Accepted（自己认可）
- [ ] 一周 dev log 完成

---

## 4. AI 协作的标准姿势

### 4.1 三种典型场景

#### 场景 A：设计新接口（用 Claude Code）

**何时**：要新增一个聚合根、Port 接口、插件能力

**Prompt 模板**：
```
项目：myCad（CAD 软件）
当前任务：<一句话描述>
请阅读以下文件理解上下文：
- docs/architecture/02-technical.md §<相关节>
- docs/architecture/05-code-skeletons.md §<相关节>
- src/<相关已有代码>

然后帮我：
1. 设计接口（C++20 头文件骨架）
2. 列出可能的实现策略与权衡
3. 给出 3-5 个验收测试场景
4. 评估是否需要写 ADR

约束：
- 遵守 ADR-0002 Domain 零依赖
- 遵守 ADR-0003 事件不可变
- 错误处理用 std::expected
```

#### 场景 B：实现已设计接口（用 DeepSeek）

**何时**：接口已设计完毕，需要写实现 + 测试

**前置准备**（Claude Code 帮你写）：
1. 在 `docs/ai-context/CONTEXT-task-NNNN.md` 写好任务上下文（用 [模板](./docs/ai-context/CONTEXT-template.md)）
2. 在 `TASKS.md` 加任务卡（用 [模板](./docs/ai-context/TASKS-template.md)）

**Prompt 模板**（给 DeepSeek）：
```
我有一个 C++ 实现任务。请按以下两份文档执行：

[粘贴 CONTEXT-task-NNNN.md 全文]

[粘贴任务卡全文]

特别注意：
- 严格遵守"硬约束"清单
- 测试用 Catch2 v3
- 风格遵守项目 .clang-format
- 生成的代码必须能通过项目 .clang-tidy

请按"工作切分"段落的子任务顺序产出。
每完成一个 SubTask 停下来等我说"继续"。
```

#### 场景 C：架构合规审查（用 Claude Code）

**何时**：DeepSeek 提了 PR，合并前

**Prompt 模板**：
```
请用 docs/architecture/03-ai-workflow.md §3.6.3 的"架构合规审查模板"
对以下 PR 做完整审查：

[粘贴 git diff 输出，或 PR URL]

输出格式按模板要求（🔴 阻塞 / 🟡 建议 / 🟢 优秀 / 📊 总评）。
特别检查：
- Tier A 类型隔离（OCCT / Qt / EnTT 不出 Adapter）
- std::expected 错误路径
- Doxygen 注释完整
- Catch2 测试覆盖
```

### 4.2 不要做的事

- ❌ 不要让 AI 一次性写 1000+ 行新代码（必崩，分 SubTask）
- ❌ 不要让 AI 自由决定架构方向（必须基于 ADR）
- ❌ 不要不审查直接合并 AI 生成的代码（即使测试 green）
- ❌ 不要把多个无关任务塞进一个 prompt
- ❌ 不要忽略 AI 提出的"我注意到一个潜在问题"（90% 是真问题）

### 4.3 一次会话的产出限制

| 工具 | 单次会话理想任务量 | 信号：该停了 |
|---|---|---|
| Claude Code | 1 个接口设计 OR 1 个 ADR OR 1 次审查 | 上下文超过 50% |
| DeepSeek | 1 个 SubTask（≤ 200 行新代码） | 输出开始重复 / 走神 |
| Copilot | 单文件内的行内补全 | 跨文件需求 |

会话之间用 `CONTEXT.md` / `TASKS.md` / git commit message 传递状态。

---

## 5. Sprint 节奏（双周一轮）

### 5.1 Sprint 启动日（每两周的周一）

**1 小时**完成：

1. 复盘上 Sprint：写 `docs/sprint-reviews/0.X.md`
   - 计划了什么 / 完成了什么 / 卡在哪
   - 估时偏差（实际 vs 计划）
   - 下 Sprint 调整哪些
2. 规划本 Sprint：从 [§六 路线图](./docs/architecture/06-roadmap.md) 复制本 Sprint 任务到 GitHub Project board
3. 给每个任务起草任务卡（Claude Code 帮你写）

### 5.2 Sprint 中（每天）

| 时段 | 内容 |
|---|---|
| 21:00-21:15 | 看任务板，挑今天做哪个 SubTask |
| 21:15-22:45 | 实际编码（Claude/DeepSeek 配合） |
| 22:45-23:00 | git commit + 写 dev log |

### 5.3 Sprint 末日（每两周的周日）

**30 分钟**完成：
- 跑全套测试（VS Test Explorer + CI）
- 检查所有任务状态（done / in_progress / blocked）
- merge 本 Sprint 所有 PR 到 main
- 给自己一个小奖励（哪怕只是 30 分钟游戏 / 一杯好咖啡）

### 5.4 月度复盘（每月最后一个周日）

**1 小时**：
- 写 `docs/monthly-reviews/2026-MM.md`
- 关键指标：commit 数、闭合 issue 数、CI green 率
- 风险扫描：[§七 风险](./docs/architecture/07-risks.md) 6 项逐一过一遍
- 心理状态自检（参考 §7.1.2 的早期信号清单）

---

## 6. 检查点与回退信号

### 6.1 Phase 0 关键检查点

| 时间 | 应该达到 | 没达到怎么办 |
|---|---|---|
| Sprint 0.1 末（Week 2） | CI 三平台 green，VS 可调试 hello-world | 暂停其他工作，专心解决工具链问题（很可能是 vcpkg 配置） |
| Sprint 0.3 末（Week 6） | 能调用 `geometryPort.makeBox(10,10,10)` 拿到 BRepHandle | 接受 OCCT 集成的复杂度比预期高，给 0.4-0.5 加 buffer |
| Sprint 0.4 末（Week 8） | 能在窗口里画一个三角形 | 渲染层是难点，可以先用最简方案（无 PBR） |
| Sprint 0.5 末（Week 10） | 能保存 → 重开 → 立方体在 | 这是 Phase 0 灵魂剧本，必须达到才能进 0.6 |
| Sprint 0.6 末（Week 12） | 内部 alpha 可邀请 1-2 朋友试用 | 即使没插件骨架，能跑就是胜利 |

### 6.2 红色警报（出现立刻停下重评）

- 🔴 连续 3 个 Sprint 估时全部 2x 超出 → 路线图过激进，重新校准
- 🔴 连续 2 周没有 commit → 心理 / 健康 / 优先级问题，立刻休息
- 🔴 同一个 bug 卡住 > 3 天 → 求助（GitHub Issue / 论坛 / 朋友）
- 🔴 CI 持续 red 超过 1 周 → 暂停新功能，专注修 CI
- 🔴 自己看 commit 记录已经看不懂为什么这么写 → 立刻文档化或重构

### 6.3 黄色警告（注意但不必停）

- 🟡 单 Sprint 估时偏差 50%+ → 正常，记录原因
- 🟡 有 PR 超过 1 周没合并 → 可能是太大了，下次拆细
- 🟡 dev log 连续 3 天没写 → 工作流偏离，重新建立习惯

---

## 7. 卡住时怎么办（按顺序尝试）

1. **休息 30 分钟**：洗澡 / 散步 / 睡觉。CAD bug 80% 在脑子清醒后立刻看到
2. **写下来**：在 dev log 中详细描述卡点，往往写到一半就想到答案
3. **问 Claude Code**：用 [§3.6.6 Bug 调试模板](./docs/architecture/03-ai-workflow.md)
4. **问 DeepSeek**：换个 AI 视角
5. **二分定位**：git bisect / 注释一半代码 / 简化输入
6. **Stack Overflow**：贴最小复现
7. **GitHub Discussion 求助**（如果项目已开源）：自己仓库或上游（OCCT、Qt、EnTT）
8. **Discord/微信群**：CAD 圈、C++ 圈
9. **绕开**：把这个任务标 blocked，做别的，让潜意识背景处理
10. **降级**：暂时用更简方案完成 MVP，复杂方案留 Phase 2

---

## 8. 心理健康基础设施

### 8.1 反孤独工具

- 每周与 1-2 个朋友视频/语音 30 分钟（不必谈 myCad，正常社交）
- 加入 1-2 个开源/CAD 社群（Discord / 微信群），做"潜水观察者"也行
- 每月写 1 篇技术博客（公开发表，让世界知道你存在）

### 8.2 进度焦虑应对

- 看 dev log 而不是看 ROADMAP（看自己走了多远，而不是还要多远）
- 每月跟 1 个月前的 commit 对比 — 永远在进步
- 接受"6 个月后可能还没 v0.1" — CAD 真的难，不是你的问题

### 8.3 红色信号（立刻寻求专业帮助）

- 持续失眠 > 2 周
- 体重快速变化 > 5kg/月
- 对所有事失去兴趣（不只是 myCad）
- 出现自伤念头

myCad 不值得你的健康。任何时候都不值得。

---

## 9. 第一个月（Phase 0 前 4 周）的"成功定义"

不是"完成 Sprint 0.1-0.2"，而是：

✅ **建立可持续的工作节奏**（每周 ≥ 4 天动手）
✅ **建立 AI 协作的肌肉记忆**（不需要查文档就能开 Claude / DeepSeek 协作）
✅ **建立 dev log 习惯**（每天记录）
✅ **CI green**（基础设施稳了）
✅ **第一个外部 commit ready**（即使没人来看，仓库是"活的"）

完成上述 5 项 = Phase 0 第一个月成功，无论实际 Sprint 进度。

---

## 10. 何时不再需要这份手册

当你能自己回答以下问题时，可以删掉本文件：
- 我下一步该做什么？
- 这个任务该用 Claude 还是 DeepSeek？
- 这个 bug 该怎么定位？
- 这个 Sprint 该不该停下来重评？

预计：Phase 0 末（Week 12）你应该有 60% 的问题能自己答；Phase 1 末（Month 12）应该 90%。

---

## 附录：常用命令速查

### 构建
```powershell
# 配置
cmake --preset vs2022-x64-debug
# 构建
cmake --build --preset vs2022-x64-debug --target mycad_app
# 测试
ctest --preset vs2022-x64-debug --output-on-failure
# 清理
cmake --build --preset vs2022-x64-debug --target clean
```

### Git
```powershell
git checkout -b feat/issue-NNN-short-name
git add <files>
git commit -m "feat(scope): short subject"
git push -u origin <branch>
gh pr create --title "..." --body "..."   # 用 GitHub CLI
```

### AI 工具
- Claude Code: <https://claude.com/claude-code>（CLI 或 Web）
- DeepSeek: <https://chat.deepseek.com>
- Copilot: VS 内 Ctrl+I 或行内 Tab

### 文档
```powershell
# 生成 API doc
cmake --build --preset vs2022-x64-debug --target docs
# 浏览
start build/vs2022-x64-debug/docs/html/index.html
```

---

> **最后修订**：2026-05（首版）
>
> 本文件 ≠ 架构文档。架构稳定，本手册根据实践不断微调。每月复盘时检查本手册哪些建议没落地，调整或删除。
