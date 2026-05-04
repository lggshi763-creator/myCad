# Changelog

本文件记录 myCad 所有用户可见的变更。格式遵循 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，版本号遵循 [SemVer](https://semver.org/lang/zh-CN/)。

## [Unreleased]

### Added
- 项目初始化：完整架构方案、ADR 文档体系、Visual Studio 工具链、自研路径策略
- 9 章架构文档（业务、技术、AI 工作流、决策、代码骨架、路线图、风险、VS 工具链、自研路径）
- 9 个 ADR（协议、Domain 零依赖、事件不可变、OCCT、OpenGL、插件 ABI、EnTT、Visual Studio、自研路径）
- AI 协同模板（CONTEXT.md / TASKS.md）
- 贡献指南、行为准则、路线图

### 计划中（Phase 0 内）
- CMake 项目骨架 + vcpkg manifest
- CMakePresets.json（VS 2022 / Linux / macOS preset）
- GitHub Actions CI（三平台）
- Domain 接口骨架（IGeometryPort、IEventStore、IConstraintSolver、IEntityRegistry、IPlugin）
- Infrastructure Adapter（OCCT、EnTT、InMemoryEventStore）
- 端到端 demo：能显示 3D 立方体并保存

详见 [ROADMAP.md](./ROADMAP.md)。

---

## 版本格式

未来 release 条目示例：

```
## [0.1.0] - YYYY-MM-DD

### Added
- 草图：直线 / 圆 / 圆弧绘制
- 草图：几何约束（重合、平行、垂直、相切）
- 草图：尺寸约束（距离、角度、半径）
- 特征：拉伸（凸台 / 切除）
- 特征：旋转
- 文件：.mycad 原生格式（基于 EventStore）
- 文件：STL / OBJ 导出
- UI：Qt 主窗口、特征树面板、属性面板
- Undo/Redo：基于事件流的撤销重做

### Changed
- ...

### Deprecated
- ...

### Removed
- ...

### Fixed
- ...

### Security
- ...
```
