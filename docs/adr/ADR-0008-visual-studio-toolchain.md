# ADR-0008: Visual Studio 2022 作为主 IDE 与构建环境

- **Status**: Accepted
- **Date**: 2026-05-03
- **Decider**: 项目维护者
- **Author**: Claude Code (drafted) / 人工 (Accepted)

## Context

myCad 是 C++20 大型 CAD 项目，依赖 OpenCASCADE / Qt 6 / EnTT / PlaneGCS 等大型 C++ 库。开发体验对个人独立开发者的产能影响巨大 — IDE 选错可能让单 sprint 的产出下降 30%-50%。

需要在项目早期（Phase 0 Sprint 0.1）就锁定主 IDE 与对应的构建配置，让所有后续 sprint 的工具链开发都基于同一基线。

不决策的代价：每次新人/AI 协助上手时重新讨论工具链 → 经验无法复用，文档分裂。

## Decision

我们决定**采用 Visual Studio 2022 Community 作为主推荐 IDE 与默认 Windows 构建环境**，配合 **CMake + vcpkg manifest 模式** 作为跨平台构建系统。

跨平台支持承诺：
- Windows：Visual Studio 2022（主推荐）
- Linux：VSCode + clangd 或 CLion（推荐）
- macOS：CLion 或 VSCode（推荐）

CI 矩阵覆盖三平台均强制 green，杜绝"VS 可编 Linux 不可编"。项目**不强制**贡献者使用特定 IDE，但所有"开箱即用"的工具配置（`.vsconfig`、`CMakePresets.json`、`.clang-format`、`.clang-tidy`、`mycad.natvis`）都以 VS 为基线设计。

## Considered Alternatives

### Option A: Visual Studio 2022 Community
- C++ 智能感知业界最强（含 IntelliCode AI 补全）
- CMake / vcpkg / Qt / Catch2 全部一等支持
- Windows 调试器最佳（Watch / Locals / Memory / Threads / TTD）
- Test Adapter for Catch2 自动发现测试
- 个人/开源/教育免费
- Windows 偏向（Linux/macOS 需切其他工具）

### Option B: VSCode + clangd + CMake Tools
- 跨平台一致体验
- 轻量、启动快
- 远程开发 / 容器开发体验最佳
- 但 IntelliSense 在 OCCT 等深度模板代码上经常失灵
- 调试器在 Windows 弱于 VS（Watch 视图深度受限）
- Test Explorer 集成需额外配置

### Option C: CLion
- 智能感知与重构能力优秀
- 跨平台一致
- 内置调试器良好
- 个人 $99/年（开源项目可申请免费）
- Windows + MSVC 调试体验略弱于 VS
- vcpkg 集成需手动

### Option D: Qt Creator
- 对 Qt 集成最佳（毕竟自家）
- 免费
- 但对纯 C++ 大型项目（CAD 内核 + ECS + OCCT）支持远弱于上述选项
- CMake 集成相对简陋

## Rationale

**Windows 是当前最优开发平台**：
- 个人开发者大多用 Windows 工作站
- OCCT 在 Windows 上的预编译二进制最完善
- Visual Studio 在 Windows 平台投资数十亿美元，C++ 体验无可超越
- VS Community 免费 = 个人开发者零成本进入

**调试体验是核心**：
- CAD 内核的 BUG 大多藏在"复杂数据结构 + 数值精度 + 状态时序"交叉处
- VS 的数据断点 / 时间旅行调试 / 内存视图在这种场景下是杀手锏
- 调试效率提升 50% = 每周节省 5-10 小时

**vcpkg 集成是关键**：
- VS 2022 17.6+ 原生集成 vcpkg manifest
- OCCT + Qt 通过 vcpkg 一键安装
- 这两点其他 IDE 都需要手动配置

**跨平台不放弃**：
- CMake + CMakePresets.json 让所有平台用同一份构建脚本
- CI 三平台并行验证
- 项目不强制 IDE，只锁构建系统

## Consequences

### Positive
- 个人开发者零工具成本（VS Community + GitHub Copilot 学生版/开源项目可申请）
- Windows 上"开 IDE → F5 调试"的开箱体验最佳
- Test Explorer 自动列出所有 Catch2 测试
- 调试 OCCT 等大型 C++ 库的体验远超其他 IDE
- 与 Qt VS Tools 配合，Qt 项目体验良好
- 可享受 Microsoft 持续投入的 C++ 工具改进

### Negative
- 偏向 Windows 平台 → Linux/macOS 贡献者需用其他 IDE（缓解：CMake 跨平台 + 文档明确支持）
- VS 安装包大（~40 GB） → 新人首次配置时间长（缓解：提供 `.vsconfig` 一键安装）
- VS 升级节奏快，可能引入回归（缓解：项目锁定 VS 2022 LTSC 17.x）
- 部分功能在 Community 版受限（如 TTD 仅 Enterprise） → 影响范围小

### Neutral
- 需要维护 `tools/vs2022.vsconfig` 配置文件
- 需要维护 `CMakePresets.json` 中的 `vs2022-*` preset
- 需要维护 `tools/visualizers/mycad.natvis` 自定义类型可视化
- 文档（[§八 vs-toolchain](../architecture/08-vs-toolchain.md)）需保持与 VS 版本同步
- CI 必须验证三平台

## Implementation Notes

### Phase 0 必须落地的工件

- `tools/vs2022.vsconfig` — VS 安装清单（一键 import）
- `CMakePresets.json` — `vs2022-x64-debug/release/asan/ubsan` 4 个 preset
- `CMakeUserPresets.json.template` — 个人覆盖模板（git ignored）
- `.editorconfig` — VS 自动识别
- `.clang-format` / `.clang-tidy` — VS 内置工具自动应用
- `tools/visualizers/mycad.natvis` — 自定义类型可视化
- `docs/architecture/08-vs-toolchain.md` — 完整文档（已就绪）
- `.githooks/pre-commit` — clang-format + clang-tidy + commit message lint

### CI 配置要点

- GitHub Actions matrix：`{ windows-2022, ubuntu-22.04, macos-13 }` × `{ Debug, Release }`
- Windows runner 自带 VS 2022 → 可直接用 `vs2022-x64-*` preset
- Linux/macOS runner 用 GCC 12+ 或 Clang 15+ 与对应 preset

### 新人引导流程

1. 安装 VS 2022 Community + 导入 `tools/vs2022.vsconfig`
2. 设置环境变量 `VCPKG_ROOT`
3. `git clone` myCad
4. VS Open → CMake → 选 `vs2022-x64-debug` preset
5. 等待首次 vcpkg 依赖编译（5-30 min）
6. F5 调试运行

详见 [§八 vs-toolchain §8.2-8.3](../architecture/08-vs-toolchain.md)。

## References

- 详细工具链文档：[§八 Visual Studio 工具链](../architecture/08-vs-toolchain.md)
- 决策清单：[§四 4.1.2 IDE 与开发环境](../architecture/04-tech-decisions.md)
- 路线图 Phase 0 Sprint 0.1：[§六 6.1 Phase 0](../architecture/06-roadmap.md)
- vcpkg 集成：[Microsoft 文档](https://learn.microsoft.com/en-us/vcpkg/)
- CMakePresets：[CMake 官方文档](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html)
