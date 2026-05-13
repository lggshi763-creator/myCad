# Sprint 0.2 — Domain 骨架（Week 3-4，2026-05-11 ~ 05-24）

> 本文件由 prompt-library.md §1 生成。执行期间请按每日步骤操作，完成即勾选验收项。
>
> **工具分工**：🟦 Claude Code（架构/接口/审查）·🟧 DeepSeek（实现/单测/样板）·🟩 人工（决策/验证）
>
> **每日编码时间上限**：2-3 小时（含 AI 等待）。周日休息，Day 1-10 覆盖 10 个工作日。
>
> **参考**：[PLAYBOOK.md](../../PLAYBOOK.md) · [ADR-0013](../adr/ADR-0013-value-object-design-philosophy.md) · [05-code-skeletons.md](../architecture/05-code-skeletons.md)

---

## 阶段目标（一句话）

所有 Domain 层接口与值对象编译通过、Doxygen 文档完整、Catch2 覆盖率 ≥ 90%（值对象）；下游 Infrastructure Adapter 可以并行开发。

---

## Sprint 验收清单（端到端可验证）

- [ ] **V1** `cmake --build --preset local-debug` 链接通过，无任何 domain/ 层头文件引用 OCCT / Qt / fmt / spdlog
- [ ] **V2** `ctest --preset local-debug` 全部 PASS，覆盖 7 个值对象 + Identity 类型 + DomainEvent
- [ ] **V3** `cmake --build --preset local-debug --target docs` 成功，`greet` / `Point2D` / `DomainEvent` 均出现在 Sphinx 渲染页
- [ ] **V4** CI Windows + Linux 两平台 × Debug + RelWithDebInfo 全部 green（macOS 暂 park）
- [ ] **V5** `clang-tidy` 在 CI 对新增文件零 warning
- [ ] **V6** 端到端剧本：构造 `Point3D{1,2,3}`，调用 `to_string()`，与 `"Point3D{x=1.000000, y=2.000000, z=3.000000}"` 比较通过
- [ ] **V7** `IGeometryPort.hpp` / `IEventStore.hpp` / `IConstraintSolver.hpp` / `IEntityRegistry.hpp` 四个接口文件均通过 grep 检查（无 `#include` OCCT/EnTT/Qt 关键字）

---

## 任务总览

（从 [06-roadmap.md §6.1 Sprint 0.2](../architecture/06-roadmap.md)）

| # | 任务 | 标签 | 估时 | 验收 | 状态 |
|---|---|---|---|---|---|
| T1 | 核心值对象（Tolerance + Point2D/3D + Vector3D + Axis3D + BoundingBox + Transform3D） | 🟧 + 🟩 | 2d | 单测覆盖 ≥ 90% | pending |
| T2 | Identity 类型（EventId / AggregateId / SketchId / Version） | 🟧 | 1d | 单测 | pending |
| T3 | DomainEvent 基类 + EventTypeRegistry | 🟦 接口 + 🟧 实现 | 2d | 单测 | pending |
| T4 | IGeometryPort 接口（含 BRepHandle / WireHandle / 错误类型，零实现） | 🟦 | 1d | 编译通过、文档生成 | pending |
| T5 | IEventStore 接口 + EventStream | 🟦 | 1d | 编译通过 | pending |
| T6 | IConstraintSolver 接口（占位，无实现） | 🟦 | 0.5d | 编译通过 | pending |
| T7 | IEntityRegistry 接口 + Component concept | 🟦 | 1d | 编译通过 | pending |
| T8 | ADR-0014+（本 Sprint 设计决策，若产生新不可逆决策） | 🟦 + 🟩 | 0.5d | docs 提交 | pending |

**估时合计**：9 d × ~2.5h = **~22.5h**（在 16-25h 双周合理范围内）

---

## Inbox 重叠项处置方案

> Sprint 启动前在 [docs/inbox.md](../inbox.md) 发现以下条目与本 Sprint 重叠，已纳入日程：

| Inbox 项 | 处置方案 | 落地位置 |
|---|---|---|
| GitHub push TLS error（阻塞 V7 CI） | Day 1 上半 warm-up 解决 | Day 1 §步骤 1 |
| vcpkg-reset-registry.ps1 | Day 1 顺便创建（10 min） | Day 1 §步骤 2 |
| 解开 `mycad.natvis` 注释化 Type entry | 随每个值对象 PR 同步解开 | Day 2-6 各 PR |
| Sprint 0.1 retrospective | Day 1 下半 wrap-up 后写 | Day 1 §步骤 4 |
| preset 改名 `vs2022-*` | 推迟，不进 Sprint 0.2 | — |
| CI paths-filter / NuGet cache | 推迟，不进 Sprint 0.2 | — |

---

## 每日任务分解

---

### Day 1（~1.5h）：热身 — Sprint 0.1 收尾

> **主用工具**：🟩 人工 + 🟦 Claude Code 辅助
>
> **前置**：无

#### 步骤

1. **修 GitHub push TLS error**

   ```powershell
   # 方案 A：改用 SSH 远端（推荐）
   git remote set-url origin git@github.com:<your-org>/myCad.git
   git push --set-upstream origin master_init

   # 方案 B：如 SSH 也不通，先确认代理
   git config --global http.proxy http://127.0.0.1:7897
   git push
   ```

   > 如仍失败，查 `git push -v` 输出，对照 [docs/devlog/2026-W19.md §1.3](../devlog/2026-W19.md) 的 TLS 处理表。

2. **创建 `tools/vcpkg-reset-registry.ps1`**（10 min，顺便清掉 inbox 项）

   ```powershell
   # 在项目根运行（PowerShell）
   @'
   # tools/vcpkg-reset-registry.ps1
   # 清除 vcpkg registry 的 unborn-master 坏状态（W19 复发 3 次的根因）
   # 用法：. .\tools\vcpkg-reset-registry.ps1
   $cache = "$env:LOCALAPPDATA\vcpkg\registries"
   Get-Process TGitCache -ErrorAction SilentlyContinue | Stop-Process -Force
   Remove-Item -Recurse -Force "$cache\git*" -ErrorAction SilentlyContinue
   Write-Host "vcpkg registry cache cleared. Next cmake will re-clone (~1-3 min, need proxy)."
   '@ | Out-File -Encoding utf8 tools\vcpkg-reset-registry.ps1
   git add tools\vcpkg-reset-registry.ps1
   ```

3. **验证 V5（docs target）和 V6（git hooks）**

   ```powershell
   # V5: docs target
   cmake --build --preset local-debug --target docs
   # 检查 build/docs/sphinx/index.html 存在，浏览器打开确认 greet() 有渲染

   # V6: git hooks
   git config core.hooksPath .githooks
   # 测试：故意写一个格式不对的 .hpp，git add + git commit，确认 pre-commit 拦截
   ```

4. **写 Sprint 0.1 retrospective**（30 min）

   ```
   【给 Claude Code 的 prompt — 可复制粘贴】

   你是 myCad 项目的复盘助手。请阅读以下文件后生成 Sprint 0.1 retrospective：

   1. docs/devlog/2026-W19.md — 本周全记录（必读）
   2. docs/architecture/06-roadmap.md §6.1 Sprint 0.1 — 验收清单
   3. docs/ai-context/prompt-library.md §4 — 复盘模板

   生成 docs/retrospectives/sprint-0.1.md，按 §4 模板结构填充。
   重点：
   - §2 "卡点与解决方案"：至少列出 W19 三大卡点（vcpkg registry / VS 18 preset 错位 / schannel CRL）
   - §4 "给下个 Sprint 的调整"：基于本 playbook Day 1-10 写出具体调整
   - 心理健康自检必须诚实填写

   不要做：不要美化数据，不要把"总体不错"这类废话写进去。
   ```

5. **commit 收尾**

   ```powershell
   git add tools/vcpkg-reset-registry.ps1 docs/retrospectives/sprint-0.1.md
   git commit -m "chore(sprint-0.1): close sprint — retro written, vcpkg-reset script added"
   git push
   ```

#### 验收

- [ ] GitHub push 成功（V7 前置解除）
- [ ] `tools/vcpkg-reset-registry.ps1` 已提交
- [ ] `cmake --build --preset local-debug --target docs` 成功（V5 ✓）
- [ ] pre-commit hook 能拦截格式违规（V6 ✓）
- [ ] `docs/retrospectives/sprint-0.1.md` 已提交

#### 卡点预案

- push TLS 仍失败 → 改用 SSH remote 或临时绕过（先在本地工作，Day 2 再推）；不要因网络问题停下 Sprint 0.2 工作
- docs target 报错 → 检查 `pip install sphinx breathe` 是否执行；Python 版本 ≥ 3.10

---

### Day 2（~2.5h）：T1 Part 1 — Tolerance.hpp + Point2D + Point3D

> **主用工具**：🟦 Claude Code 审查接口 + 🟧 DeepSeek 实现
>
> **前置**：Day 1 完成（或跳过，只要 local build 正常）
>
> **设计依据**：[ADR-0013](../adr/ADR-0013-value-object-design-philosophy.md)（全部 9 条决策，无例外）

#### 步骤

1. **先用 Claude Code 生成接口骨架**（15 min）

   ```
   【给 Claude Code 的 prompt — 可复制粘贴】

   你是 myCad 项目的架构助手。请按以下规格生成三个头文件的骨架（仅头文件，不写 .cpp）：

   【必读文件】
   1. docs/adr/ADR-0013-value-object-design-philosophy.md — 9 条设计决策（必须全部遵守）
   2. src/domain/shared/include/mycad/domain/Hello.hpp — Doxygen 注释风格样板

   【输出文件 1】src/domain/shared/include/mycad/domain/Tolerance.hpp
   内容：
   - namespace mycad::domain
   - inline constexpr double kDefaultEpsilon = 1e-9;（Doxygen 注释：brief 英文 + 详情中文）
   - [[nodiscard]] constexpr bool nearEqual(double a, double b) noexcept;（两个重载）
   - @file / @brief / @param / @return 全部齐全

   【输出文件 2】src/domain/shared/include/mycad/domain/Point2D.hpp
   内容：
   - namespace mycad::domain
   - struct Point2D { double x; double y; }（传值，< 32B，ADR-0013 决策 1）
   - 构造函数 noexcept，explicit 或聚合初始化（选聚合，更 C++ 风格）
   - [[nodiscard]] friend bool operator==(Point2D a, Point2D b) noexcept;（走 nearEqual）
   - [[nodiscard]] Point2D translated(Point2D delta) const noexcept;
   - [[nodiscard]] std::string to_string(Point2D p);（自由函数，同命名空间，不 noexcept）
   - Doxygen 注释：每个公共实体一套（/// @brief 英文 + 中文详情）

   【输出文件 3】src/domain/shared/include/mycad/domain/Point3D.hpp
   同 Point2D 结构，改为三维（x, y, z），加 distanceTo(Point3D) noexcept 方法。

   【不要做】
   - 不要在头文件写任何实现（除 constexpr/inline 外）
   - 不要 #include OCCT / Qt / fmt / spdlog / <format>
   - 不要 operator<< （ADR-0013 决策 9）
   - 不要 std::hash 特化（ADR-0013 决策 7）
   - 不要 operator<=> 给浮点类（ADR-0013 决策 3）
   - 头文件路径必须是 src/domain/shared/include/mycad/domain/Xxx.hpp
   ```

2. **把骨架写入文件**（Claude Code 直接用 Write 工具创建）

3. **交给 DeepSeek 实现 .cpp + 单测**

   ```
   【给 DeepSeek 的 prompt — 可复制粘贴】

   你是 myCad 项目的 C++20 实现工程师。本次任务：实现 Tolerance / Point2D / Point3D 的 .cpp 文件 + Catch2 单测。

   【必读文件（全部粘贴给你，不要自行推断）】
   - src/domain/shared/include/mycad/domain/Tolerance.hpp（刚生成的骨架）
   - src/domain/shared/include/mycad/domain/Point2D.hpp（刚生成的骨架）
   - src/domain/shared/include/mycad/domain/Point3D.hpp（刚生成的骨架）
   - src/domain/shared/include/mycad/domain/Hello.hpp（实现风格参考）
   - src/domain/shared/Hello.cpp（纯 std::string 拼接风格参考）
   - tests/domain/shared/hello_test.cpp（Catch2 测试风格参考）

   【输出 1】src/domain/shared/Tolerance.cpp
   - 实现 nearEqual 两个重载（inline constexpr 可在头文件，但提供 .cpp 方便 explicit instantiation）
   - 如果 nearEqual 是 constexpr，不需要 .cpp；输出空文件或 #include 即可

   【输出 2】src/domain/shared/Point2D.cpp
   - 实现 operator== / translated / to_string
   - to_string 格式："Point2D{x=1.500000, y=2.500000}"（std::to_string 默认精度）
   - 纯 std::string 拼接，out.reserve(48)，不用 <format> / fmt

   【输出 3】src/domain/shared/Point3D.cpp
   - 实现 operator== / translated / distanceTo / to_string
   - to_string 格式："Point3D{x=1.0, y=2.0, z=3.0}"（同上精度）

   【输出 4】tests/domain/shared/value_objects_test.cpp（追加到 hello_test.cpp 或新建文件）
   Catch2 TEST_CASE 覆盖：
   - Tolerance: nearEqual(1.0, 1.0) == true; nearEqual(1.0, 1.0 + 1e-9) 边界; nearEqual(1.0, 1.0 + 1e-8) == false
   - Point2D: 默认构造; translated; operator==（epsilon 边界）; to_string 格式
   - Point3D: 同上 + distanceTo(已知距离)
   每个 TEST_CASE 覆盖 happy path + edge case（NaN、极大值 1e15）
   覆盖率目标 ≥ 90%

   【硬约束】
   - 所有 #include 只用 <string> <string_view> <cmath> <array> 等标准库
   - 不用 <format> / fmt / spdlog / Qt / OCCT
   - 所有方法 noexcept 除 to_string（to_string 返回 std::string 可能 OOM，不标 noexcept）
   - [[nodiscard]] 标在所有返回值有意义的方法
   - 测试文件用 TEST_CASE / REQUIRE / CHECK 宏，不用 SCENARIO / GIVEN（保持项目风格）

   【不要做】
   - 不要写 CMakeLists.txt（已有，不需要改动）
   - 不要在 Domain 层引入任何外部依赖
   - 不要把 to_string 设计成序列化格式（仅用于 debug/log）
   ```

4. **Claude Code 审查 + 解开 natvis**（15 min）

   检查 DeepSeek 输出，参照 ADR-0013 Code Review Checklist；同时在 `tools/visualizers/mycad.natvis` 里解开 `Point2D` / `Point3D` 的注释化 Type entry（字段名对应 `x` / `y` / `z`）。

5. **commit**

   ```powershell
   git add src/domain/shared/ tests/domain/shared/ tools/visualizers/mycad.natvis
   git commit -m "feat(domain): add Tolerance, Point2D, Point3D value objects (ADR-0013)"
   ```

#### 验收

- [ ] `ctest --preset local-debug` Tolerance + Point2D + Point3D 全 PASS
- [ ] `grep -r "#include" src/domain/shared/` 无 OCCT / Qt / fmt
- [ ] natvis 中 Point2D / Point3D Type entry 已解开注释
- [ ] clang-format 不报违规（`git diff HEAD --name-only | xargs clang-format --dry-run -Werror`）

#### 卡点预案

- `distanceTo` 需要 `<cmath>` → 允许，std::sqrt 是标准库
- `std::to_string` 精度问题（输出 `1.000000` 而非 `1`）→ 这是预期行为，to_string 只用于 debug

---

### Day 3（~2.5h）：T1 Part 2 — Vector3D + Axis3D + BoundingBox + Transform3D

> **主用工具**：🟧 DeepSeek 实现 + 🟦 Claude Code 审查
>
> **前置**：Day 2 完成（Point3D.hpp 已存在，Vector3D 依赖它）

#### 步骤

1. **Claude Code 生成接口骨架**（20 min）

   ```
   【给 Claude Code 的 prompt — 可复制粘贴】

   你是 myCad 项目的架构助手。请生成四个头文件骨架。

   【必读】
   1. docs/adr/ADR-0013-value-object-design-philosophy.md — 9 条决策（全遵守）
   2. src/domain/shared/include/mycad/domain/Point3D.hpp — 已有，作为 Axis3D / BoundingBox 的依赖

   【输出文件 1】src/domain/shared/include/mycad/domain/Vector3D.hpp
   - struct Vector3D { double x, y, z; }（24B，传值）
   - [[nodiscard]] double dot(Vector3D v) const noexcept;
   - [[nodiscard]] Vector3D cross(Vector3D v) const noexcept;
   - [[nodiscard]] double length() const noexcept;
   - [[nodiscard]] Vector3D normalized() const noexcept;（长度为零时返回 {0,0,0}）
   - operator==（走 nearEqual）、to_string 自由函数

   【输出文件 2】src/domain/shared/include/mycad/domain/Axis3D.hpp
   - struct Axis3D { Point3D origin; Vector3D direction; }（48B，传 const ref）
   - Doxygen 注释：单位方向向量（调用方负责传归一化向量，Axis3D 不强制归一化）
   - operator==（origin + direction 均走 nearEqual）、to_string 自由函数

   【输出文件 3】src/domain/shared/include/mycad/domain/BoundingBox.hpp
   - struct BoundingBox { Point3D min; Point3D max; }（48B，传 const ref）
   - [[nodiscard]] bool contains(Point3D p) const noexcept;
   - [[nodiscard]] bool intersects(const BoundingBox& other) const noexcept;
   - [[nodiscard]] BoundingBox expanded(double margin) const noexcept;
   - operator==、to_string 自由函数

   【输出文件 4】src/domain/shared/include/mycad/domain/Transform3D.hpp
   - struct Transform3D（128B，传 const ref）
   - 内部存 std::array<double, 16>（行优先 4×4 矩阵）
   - static [[nodiscard]] Transform3D identity() noexcept;
   - [[nodiscard]] Point3D apply(Point3D p) const noexcept;
   - [[nodiscard]] Vector3D apply(Vector3D v) const noexcept;
   - [[nodiscard]] Transform3D composed(const Transform3D& other) const noexcept;
   - operator==（逐元素 nearEqual）、to_string 自由函数（输出矩阵每行）

   【不要做】
   - 不要引入 Eigen / glm / 任何线性代数库（domain 零依赖，ADR-0002）
   - 不要 operator<< / std::hash / operator<=>（浮点类，ADR-0013 决策 3/7/9）
   - 不要在头文件里实现非 constexpr 方法
   ```

2. **DeepSeek 实现 .cpp + 单测**

   ```
   【给 DeepSeek 的 prompt — 可复制粘贴】

   你是 myCad 项目 C++20 工程师。实现 Vector3D / Axis3D / BoundingBox / Transform3D 的 .cpp + Catch2 单测。

   【必读（全部粘贴给你）】
   - 4 个头文件骨架（刚生成）
   - src/domain/shared/include/mycad/domain/Point3D.hpp
   - src/domain/shared/include/mycad/domain/Tolerance.hpp
   - src/domain/shared/Point3D.cpp（to_string 实现风格参考）
   - tests/domain/shared/value_objects_test.cpp（测试风格参考）

   【输出】src/domain/shared/ 下 4 个 .cpp + 追加到 tests/domain/shared/value_objects_test.cpp

   Catch2 TEST_CASE 覆盖重点：
   - Vector3D: dot/cross/length/normalized（长度为零的 normalized）
   - Axis3D: 构造 + operator==
   - BoundingBox: contains（边界点）/ intersects（恰好相接 vs 不相交）/ expanded
   - Transform3D: identity().apply(point) == point; composed(a, b) 乘法结合律（单测两个旋转矩阵）

   【硬约束】与 Day 2 相同。不要引入 Eigen / glm / 任何数学库。
   Transform3D 的矩阵乘法请手写（16 次乘加），不复杂。
   ```

3. **审查 + natvis 解开**（15 min）

   解开 natvis 中 `Vector3D` / `Axis3D` / `BoundingBox` / `Transform3D` 的注释化 Type entry。

4. **commit**

   ```powershell
   git add src/domain/shared/ tests/domain/shared/ tools/visualizers/mycad.natvis
   git commit -m "feat(domain): add Vector3D, Axis3D, BoundingBox, Transform3D (ADR-0013)"
   ```

#### 验收

- [ ] `ctest --preset local-debug` 全 PASS（含 Day 2 + Day 3 新增测试）
- [ ] Transform3D identity + composed 测试通过（检验手写矩阵乘法正确性）
- [ ] natvis 4 个 Type entry 已解开

#### 卡点预案

- Transform3D 矩阵乘法出错 → 用已知旋转矩阵（绕 Z 轴 90°）验证 `apply(Point3D{1,0,0})` 结果是否 `≈{0,1,0}`
- `normalized()` 零向量行为 → 返回 `{0,0,0}` 已在接口骨架中约定，单测覆盖即可

---

### Day 4（~2h）：T2 — Identity 类型

> **主用工具**：🟧 DeepSeek 实现 + 🟦 Claude Code 快速审查
>
> **前置**：T1 全部完成（编译通过即可）

#### 步骤

1. **Claude Code 生成接口骨架**（15 min）

   ```
   【给 Claude Code 的 prompt — 可复制粘贴】

   你是 myCad 项目架构助手。生成 Identity 类型的头文件骨架。

   【必读】
   1. docs/adr/ADR-0013-value-object-design-philosophy.md — 决策 3 / 7（ID 类型用 <=> default + 提供 std::hash）
   2. src/domain/shared/include/mycad/domain/Hello.hpp — 注释风格参考

   【输出文件 1】src/domain/shared/include/mycad/domain/EventId.hpp
   - using backing type: std::array<std::uint8_t, 16>（16B UUID，传值）
   - struct EventId { std::array<std::uint8_t, 16> bytes{}; };
   - auto operator<=>(const EventId&) const = default;（bitwise 比较 OK）
   - static [[nodiscard]] EventId generate() noexcept;（占位：返回全零，Phase 1 再接 UUID 库）
   - [[nodiscard]] std::string to_string(EventId id);（自由函数，hex 格式 "xxxxxxxx-xxxx-..."）
   - std::hash 特化（在 namespace std 里，ADR-0013 决策 7）

   【输出文件 2】src/domain/shared/include/mycad/domain/AggregateId.hpp
   同 EventId 结构，改名 AggregateId。

   【输出文件 3】src/domain/shared/include/mycad/domain/SketchId.hpp
   同 EventId 结构，改名 SketchId。

   【输出文件 4】src/domain/shared/include/mycad/domain/Version.hpp
   - struct Version { std::uint64_t value{0}; };（8B，传值）
   - auto operator<=>(const Version&) const = default;
   - [[nodiscard]] Version next() const noexcept;（返回 Version{value + 1}）
   - [[nodiscard]] std::string to_string(Version v);（自由函数，"v42"格式）
   - std::hash 特化

   【不要做】
   - 不要引入 UUID 库（generate() 占位返回全零即可，Phase 1 再扩展）
   - 不要 operator<<
   ```

2. **DeepSeek 实现 .cpp + 单测**

   ```
   【给 DeepSeek 的 prompt — 可复制粘贴】

   实现 EventId / AggregateId / SketchId / Version 的 .cpp + Catch2 单测。

   【必读（全部粘贴给你）】
   - 4 个头文件骨架（刚生成）
   - tests/domain/shared/value_objects_test.cpp（风格参考）

   【输出】src/domain/shared/ 下 4 个 .cpp + 追加 tests/domain/shared/identity_test.cpp

   Catch2 TEST_CASE 覆盖：
   - EventId: 两个默认构造的 EventId 相等（都是全零）; generate() 返回合法对象; to_string 格式（16 hex bytes + 连字符分组）
   - AggregateId / SketchId: 同 EventId（复用测试结构）
   - Version: next() 递增; to_string 格式"v0"/"v42"; operator<=>（v0 < v1 < v2）
   - hash: std::unordered_map<EventId, int> 能正常插入/查找

   【硬约束】to_string 的 hex 输出用 std::array + 手写 hex 转换（不用 <format> / sprintf）。
   Version to_string 用 "v" + std::to_string(value)。
   ```

3. **commit**

   ```powershell
   git add src/domain/shared/ tests/domain/shared/
   git commit -m "feat(domain): add Identity types — EventId, AggregateId, SketchId, Version"
   ```

#### 验收

- [ ] `ctest --preset local-debug` 全 PASS
- [ ] `std::unordered_map<EventId, int>` 在测试中正常使用（hash 正常工作）
- [ ] Version::next() 单测通过

#### 卡点预案

- hex to_string 容易写错（高/低 nibble 搞反）→ 用 `static char kHex[] = "0123456789abcdef"` 逐 byte 处理，单测验证 `\x00\x01...\x0f` 的已知输出

---

### Day 5-6（~2.5h × 2）：T3 — DomainEvent 基类 + EventTypeRegistry

> **主用工具**：🟦 Claude Code 设计接口 + 🟧 DeepSeek 实现
>
> **前置**：T1 + T2 完成（EventId 已存在）
>
> **注意**：这是 Sprint 0.2 最复杂的任务，拆成 Day 5（接口设计）和 Day 6（实现 + 测试）

#### Day 5：接口设计（🟦 Claude Code，不写实现）

```
【给 Claude Code 的 prompt — 可复制粘贴】

你是 myCad 项目架构助手。为 DomainEvent 基类 + EventTypeRegistry 设计接口骨架。

【必读】
1. docs/adr/ADR-0003-events-immutable.md — 事件不可变 + 过去时命名
2. docs/adr/ADR-0002-domain-zero-deps.md — domain 零外部依赖
3. src/domain/shared/include/mycad/domain/EventId.hpp — 已有，基类依赖它
4. docs/architecture/05-code-skeletons.md §5.2 DomainEvent — 骨架样板（重点参考）

【输出文件 1】src/domain/shared/include/mycad/domain/DomainEvent.hpp
要求：
- namespace mycad::domain
- class DomainEvent（抽象基类，不可复制，可移动）
- 私有字段：EventId eventId_; std::uint64_t occurredAt_;（Unix ms，不用 std::chrono，ADR-0002）
- 公共接口（全 noexcept 除 typeName）：
  - [[nodiscard]] EventId eventId() const noexcept;
  - [[nodiscard]] std::uint64_t occurredAt() const noexcept;
  - [[nodiscard]] virtual std::string_view typeName() const noexcept = 0;（子类必须实现）
  - virtual ~DomainEvent() = default;
- protected 构造（子类调用）：DomainEvent(EventId id, std::uint64_t occurredAt) noexcept;
- 宏 MYCAD_DOMAIN_EVENT(ClassName) — 生成 typeName() 的 override（返回 #ClassName 字符串字面量）
- Doxygen 注释齐全

【输出文件 2】src/domain/shared/include/mycad/domain/EventTypeRegistry.hpp
要求：
- namespace mycad::domain
- class EventTypeRegistry（单例，懒加载，double-checked locking 或 static local）
- 注册：void registerType(std::string_view typeName, FactoryFn factory);
- 查找：[[nodiscard]] std::optional<FactoryFn> findFactory(std::string_view typeName) const;
- FactoryFn = std::function<std::unique_ptr<DomainEvent>(EventId, std::uint64_t, std::string_view payload)>
- 宏 MYCAD_REGISTER_EVENT(ClassName) — auto-register 到 EventTypeRegistry::instance()（用 static 初始化技巧）

【不要做】
- 不要用 std::chrono（不可用，ADR-0002 spirit；用 std::uint64_t Unix ms）
- 不要 <format> / fmt
- 不要在接口文件里写实现（宏除外）
- 不要引入反射库
- EventTypeRegistry 不需要线程安全（Phase 0 单线程，Phase 1 再加锁）—— 如果加锁需要 ADR-0014，暂时不做
```

#### Day 6：实现 + 单测（🟧 DeepSeek）

```
【给 DeepSeek 的 prompt — 可复制粘贴】

实现 DomainEvent 基类 + EventTypeRegistry，并写 Catch2 单测。

【必读（全部粘贴给你）】
- src/domain/shared/include/mycad/domain/DomainEvent.hpp（刚生成骨架）
- src/domain/shared/include/mycad/domain/EventTypeRegistry.hpp（刚生成骨架）
- src/domain/shared/include/mycad/domain/EventId.hpp
- tests/domain/shared/value_objects_test.cpp（风格参考）

【输出 1】src/domain/shared/DomainEvent.cpp
- 实现 protected 构造 + eventId() / occurredAt()
- typeName() 是 pure virtual，不需要实现

【输出 2】src/domain/shared/EventTypeRegistry.cpp
- 实现 instance()（static local，线程安全足够）
- 实现 registerType / findFactory

【输出 3】tests/domain/shared/domain_event_test.cpp
TEST_CASE 覆盖：
1. 定义一个具体事件类（SketchCreatedEvent，使用 MYCAD_DOMAIN_EVENT 宏），确认 typeName() 返回 "SketchCreatedEvent"
2. 用 MYCAD_REGISTER_EVENT 注册，EventTypeRegistry::instance().findFactory("SketchCreatedEvent") 返回有值
3. 未注册类型 findFactory 返回 std::nullopt
4. 事件字段不可变（尝试修改 eventId_ 应不通过编译 — 用 static_assert 或 std::is_const 验证）
5. 事件继承链：SketchCreatedEvent is-a DomainEvent（dynamic_cast 通过）

【硬约束】
- 不要 std::chrono / <format> / fmt
- FactoryFn 调用时传入 payload 为空字符串即可（Phase 1 再接序列化）
- 宏展开后不应有任何 extern 或 global mutable state 除 registry 本身
```

**commit（Day 6 结束）**

```powershell
git add src/domain/shared/ tests/domain/shared/
git commit -m "feat(domain): add DomainEvent base + EventTypeRegistry (ADR-0003)"
```

#### 验收

- [ ] `ctest --preset local-debug` 全 PASS（含 domain_event_test）
- [ ] MYCAD_DOMAIN_EVENT 宏 + MYCAD_REGISTER_EVENT 宏均能正常展开
- [ ] grep 确认 DomainEvent.hpp 无 std::chrono / <format>

#### 卡点预案

- `static local` 单例初始化顺序问题（MYCAD_REGISTER_EVENT 在 main 之前运行）→ 确保 `EventTypeRegistry::instance()` 返回的是 local static，不是 global object；fiasco-safe
- `std::function` 在 Domain 层 → 仅用 `std::function`，是标准库，ADR-0002 允许

---

### Day 7（~2h）：T4 — IGeometryPort 接口

> **主用工具**：🟦 Claude Code（全程，纯接口设计，无 DeepSeek 参与）
>
> **前置**：T1 完成（需要 Point3D / Vector3D / Axis3D / BoundingBox / Transform3D 头文件）

#### 步骤

```
【给 Claude Code 的 prompt — 可复制粘贴】

你是 myCad 项目架构助手。请把 docs/architecture/05-code-skeletons.md §5.1 的 IGeometryPort 骨架
直接落地为文件，并补全所有 Doxygen 注释。

【必读】
1. docs/architecture/05-code-skeletons.md §5.1 — IGeometryPort 完整骨架（直接抄，不要改接口）
2. docs/adr/ADR-0002-domain-zero-deps.md — domain 零外部依赖
3. src/domain/shared/include/mycad/domain/ 下已有的值对象头文件

【输出文件】src/domain/shared/include/mycad/domain/IGeometryPort.hpp
要求：
- 完全按 §5.1 骨架，不新增/删减接口
- 补全每个方法的 Doxygen（@brief 英文 + 中文参数说明 + @return + @throws + @thread-safe 标注）
- BRepHandle / WireHandle / GeomError / GeomResult / TessellationParams 全部在此文件定义
- #include 只允许标准库 + 已有 mycad domain 头文件

【同时输出】tests/domain/shared/geometry_port_test.cpp
- 仅编译测试（不需要运行时测试，没有实现）：
  TEST_CASE("IGeometryPort compiles") {
    // static_assert that BRepHandle is trivially copyable
    // static_assert that WireHandle is trivially copyable
    // Check GeomResult<BRepHandle> is instantiable
  }

【不要做】
- 不要写任何 IGeometryPort 方法的实现（纯虚接口，Sprint 0.3 才实现 Adapter）
- 不要引入 OCCT 头文件（连一行都不行）
- 不要改变 §5.1 的接口签名
```

**commit**

```powershell
git add src/domain/shared/include/mycad/domain/IGeometryPort.hpp tests/domain/shared/geometry_port_test.cpp
git commit -m "feat(domain): add IGeometryPort interface skeleton (Sprint 0.3 will implement)"
```

#### 验收

- [ ] `cmake --build --preset local-debug` 链接通过（纯接口，无实现）
- [ ] `grep -i "occt\|BRep_\|TopoDS_\|Standard_" src/domain/shared/include/mycad/domain/IGeometryPort.hpp` 输出为空
- [ ] Doxygen 在 `make docs` 后渲染出 IGeometryPort 页面

#### 卡点预案

- §5.1 提到 `SketchSnapshot` 但 Sprint 0.2 还没这个类型 → 改为 `class SketchSnapshot; // forward declaration`，Sprint 1.A 再定义，不阻塞编译

---

### Day 8（~2h）：T5 + T6 — IEventStore + IConstraintSolver

> **主用工具**：🟦 Claude Code（纯接口）
>
> **前置**：T3 完成（DomainEvent 已存在）

#### 步骤

```
【给 Claude Code 的 prompt — 可复制粘贴】

你是 myCad 项目架构助手。生成两个接口头文件。

【必读】
1. docs/architecture/05-code-skeletons.md §5.3 IEventStore + EventStream — 骨架样板
2. src/domain/shared/include/mycad/domain/DomainEvent.hpp
3. src/domain/shared/include/mycad/domain/AggregateId.hpp
4. src/domain/shared/include/mycad/domain/Version.hpp

【输出文件 1】src/domain/shared/include/mycad/domain/IEventStore.hpp
按 §5.3 骨架：
- EventStream（聚合的所有历史事件 + 最新 Version）
- class IEventStore（纯虚）：
  - append(AggregateId, std::span<std::unique_ptr<DomainEvent>>, Version expectedVersion) → std::expected<Version, StoreError>
  - load(AggregateId, Version from) → std::expected<EventStream, StoreError>
  - subscribe(AggregateId, Callback) → SubscriptionHandle
- Doxygen 注释齐全（@brief 英文 + 中文 @param + @return）

【输出文件 2】src/domain/shared/include/mycad/domain/IConstraintSolver.hpp
占位接口（Phase 1.A2 Sprint 才实现 PlaneGCS），最小化：
- namespace mycad::domain
- class IConstraintSolver { public: virtual ~IConstraintSolver() = default; };（仅占位）
- Doxygen @brief "Placeholder for constraint solver port. Implemented in Sprint 1.A2."

【不要做】
- 不要写任何实现
- 不要引入 PlaneGCS / Eigen 头文件
- IConstraintSolver 不要加任何方法（Phase 1 再设计，避免过早决策）
```

**commit**

```powershell
git add src/domain/shared/include/mycad/domain/IEventStore.hpp src/domain/shared/include/mycad/domain/IConstraintSolver.hpp
git commit -m "feat(domain): add IEventStore + IConstraintSolver interfaces"
```

#### 验收

- [ ] 两个文件编译通过
- [ ] IConstraintSolver.hpp 只有析构函数（`wc -l` 应 < 30 行）

---

### Day 9（~2.5h）：T7 — IEntityRegistry + Component concept

> **主用工具**：🟦 Claude Code 设计 + 🟧 DeepSeek 补单测
>
> **前置**：T1 完成

#### 步骤

```
【给 Claude Code 的 prompt — 可复制粘贴】

你是 myCad 项目架构助手。生成 IEntityRegistry 接口 + Component concept。

【必读】
1. docs/architecture/05-code-skeletons.md §5.6 IEntityRegistry — 骨架样板
2. docs/adr/ADR-0007-entt-as-ecs.md — EnTT 仅由 infrastructure/ecs/ 持有，domain 不见 EnTT

【输出文件】src/domain/shared/include/mycad/domain/IEntityRegistry.hpp
按 §5.6 骨架：
- concept Component<T>（可 trivially_copyable + sizeof <= 1024 的限制）
- using EntityId = std::uint64_t;
- class IEntityRegistry（纯虚）：
  - createEntity() → EntityId
  - destroyEntity(EntityId) noexcept
  - template <Component T> emplace(EntityId, T&&) → T&
  - template <Component T> get(EntityId) → T*（不存在返回 nullptr）
  - template <Component T> remove(EntityId) noexcept
  - hasEntity(EntityId) noexcept → bool
- Doxygen 注释：@brief 英文 + @tparam 说明 concept 约束

【不要做】
- 不要 #include <entt/...>（domain 零依赖 ADR-0002）
- 不要在接口里出现 entt::entity / entt::registry
```

**DeepSeek 补编译测试**

```
【给 DeepSeek 的 prompt — 可复制粘贴】

为 IEntityRegistry.hpp 写编译期测试 tests/domain/shared/entity_registry_test.cpp：

【必读（粘贴给你）】src/domain/shared/include/mycad/domain/IEntityRegistry.hpp

TEST_CASE("Component concept satisfied by trivial struct") {
  struct Pos { float x, y; };
  static_assert(mycad::domain::Component<Pos>);
}
TEST_CASE("Component concept rejected by non-copyable") {
  struct Big { std::array<char, 2048> data; };
  static_assert(!mycad::domain::Component<Big>);
}
// IEntityRegistry 仅编译不运行（纯虚）— 无运行时测试

不要写任何 EnTT 头文件。
```

**commit**

```powershell
git add src/domain/shared/include/mycad/domain/IEntityRegistry.hpp tests/domain/shared/entity_registry_test.cpp
git commit -m "feat(domain): add IEntityRegistry interface + Component concept (ADR-0007)"
```

#### 验收

- [ ] 两个 `static_assert` 编译通过
- [ ] `grep -r "entt" src/domain/` 输出为空

---

### Day 10（~2h）：T8 + Sprint 收尾

> **主用工具**：🟦 Claude Code + 🟩 人工
>
> **前置**：T1-T7 全部完成

#### 步骤

1. **评估是否需要新 ADR**（15 min 回顾）

   回顾 Day 1-9 中是否产生了任何"不可逆设计决策"未被现有 ADR 覆盖：

   - DomainEvent 注册宏设计（MYCAD_REGISTER_EVENT） → 若与 ADR-0003 骨架有出入，写 **ADR-0014**
   - EventTypeRegistry 线程安全策略 → 若决定了具体方案，写 **ADR-0015**
   - 若上述均未产生新决策，T8 = 0 分钟（零 ADR 也是合法结果）

   ```
   【给 Claude Code 的 prompt（仅当需要 ADR 时使用）】

   你是 myCad 项目架构助手。请为以下设计决策起草一份 ADR：

   决策主题：<从上述评估中找出的主题>
   ADR 编号：ADR-0014（或 0015）
   文件路径：docs/adr/ADR-0014-<kebab-case-title>.md

   【必读】
   1. docs/adr/ADR-0013-value-object-design-philosophy.md — ADR 格式样板（完全照此格式）
   2. 本 Sprint 相关实现文件（粘贴具体内容）

   格式要求：Status / Date / Context / Decision / Considered Alternatives / Rationale / Consequences / Implementation Notes
   不要做：不要发明新决策，仅记录 Sprint 0.2 实际做出的选择。
   ```

2. **全面 CI 验证**（30 min）

   ```powershell
   # 本地 smoke check
   cmake --preset local-debug
   cmake --build --preset local-debug
   ctest --preset local-debug --output-on-failure

   # docs 生成
   cmake --build --preset local-debug --target docs

   # grep 守护（手动跑 CI 规则）
   grep -rn "#include" src/domain/ | grep -E "occt|entt|Qt|fmt|spdlog|format>" && echo "VIOLATION" || echo "domain clean"

   # push 触发 CI
   git push
   ```

3. **更新 devlog 2026-W20 / W21**

   在 `docs/devlog/2026-W20.md` 和（如果 Sprint 跨周）`docs/devlog/2026-W21.md` 记录本周产出、卡点、教训。

4. **更新 inbox.md**

   把 Day 1 处理过的项打勾（push TLS、vcpkg-reset、retrospective、natvis），把推迟项添加理由。

5. **最终 commit + 打标签**

   ```powershell
   git add docs/
   git commit -m "docs(sprint-0.2): devlog W20/W21, inbox cleanup, ADR-0014 (if applicable)"
   git tag sprint-0.2-done
   git push --tags
   ```

#### 验收

- [ ] 本地 `ctest` 全 PASS（所有 T1-T7 测试）
- [ ] `cmake --build --target docs` 成功，`IGeometryPort` / `DomainEvent` / `Point2D` 等均出现在 Sphinx 渲染
- [ ] CI Windows + Linux 四个 job green（push 后确认）
- [ ] `grep -rn "#include" src/domain/` 无 OCCT / EnTT / Qt / fmt / spdlog / `<format>`
- [ ] `docs/devlog/2026-W20.md`（或 W21）已写
- [ ] inbox.md 已更新
- [ ] git tag `sprint-0.2-done` 已推送

---

## 本 Sprint 必须产出的 ADR（如有）

| ADR 编号 | 主题 | 何时写 | 起草者 | 触发条件 |
|---|---|---|---|---|
| ADR-0014 | DomainEvent 注册机制（宏 vs 工厂 vs 静态初始化） | Day 6 后（若产生新决策） | 🟦 Claude Code 起草，🟩 人工接受 | 实现 MYCAD_REGISTER_EVENT 宏时若偏离 ADR-0003 §2.3 |
| ADR-0015 | EventTypeRegistry 线程安全策略 | Day 6 后（若决定加锁） | 🟦 Claude Code 起草 | 若接受加锁方案（Phase 0 单线程可能不需要） |

> 若 Sprint 0.2 结束时以上两个 ADR 均未产生，代表实现完全遵循现有 ADR，也是理想结果。

---

## Sprint 末验收清单

- [ ] 所有任务总览中 T1-T8 状态为 done
- [ ] CI Windows + Linux 两平台 × 2 build types = 4 个 job 全部 green（macOS park，不计）
- [ ] `src/domain/` 下零 OCCT / EnTT / Qt / fmt / spdlog / `<format>` 引用（grep 确认）
- [ ] 7 个值对象 + Identity 类型 + DomainEvent：Catch2 单测覆盖率 ≥ 90%
- [ ] 四个接口（IGeometryPort / IEventStore / IConstraintSolver / IEntityRegistry）编译通过，文档生成
- [ ] `tools/visualizers/mycad.natvis`：Point2D / Point3D / Vector3D / EventId 的 Type entry 均已解开注释
- [ ] `docs/devlog/2026-W20.md`（和 W21 如适用）完整记录
- [ ] `docs/retrospectives/sprint-0.1.md` 已提交（Day 1 完成）
- [ ] `docs/sprints/sprint-0.2-playbook.md`（本文件）所有验收项已勾选
- [ ] git tag `sprint-0.2-done` 已推送

---

## 给下个 Sprint 的预热

**Sprint 0.3 主题**：Infrastructure Adapter（InMemoryEventStore + EnttRegistry + OcctGeometryAdapter 最小实现）

**Sprint 0.3 启动前需要准备**：

1. **确认 OCCT 能通过 vcpkg 在本地构建**（冷 build 约 30-60 分钟）：
   ```powershell
   # 在 Sprint 0.2 末或 0.3 Day 1 前运行一次
   cmake --preset local-debug  # vcpkg 会尝试编 opencascade
   ```
   如果 OCCT 编译失败（常见于 schannel CRL），立即运行 `tools/vcpkg-rescue.ps1`。

2. **阅读 ADR-0004**（OcctGeometryAdapter 架构边界）+ **ADR-0007**（EnttRegistry 实现约束），作为 Sprint 0.3 设计输入。

3. **Sprint 0.3 估时预警**：Sprint 0.3 含真实 OCCT 集成，工具链风险最高，建议在 roadmap 预留 **+50% buffer**（实际可能 4-6 天而非 2 天）。Day 1 优先验证 OCCT 能跑，再排其他任务。

---

> **最后更新**：2026-05-13（Sprint 0.2 启动，Claude Code 生成）
