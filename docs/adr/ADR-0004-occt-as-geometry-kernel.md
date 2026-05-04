# ADR-0004: 采用 OpenCASCADE 作为几何内核

- **Status**: Accepted
- **Date**: 2026-05-03
- **Decider**: 项目维护者
- **Author**: Claude Code (drafted) / 人工 (Accepted)

## Context

myCad 是 CAD 软件，几何内核是核心子系统 — 负责：
- 基础形体（Box、Cylinder、Sphere）
- 拉伸、旋转、扫掠、放样
- 布尔运算（Union / Cut / Intersect）
- 倒角、圆角、抽壳
- B-Rep 拓扑数据结构
- STEP / IGES 格式互操作
- 网格化（用于渲染）

这是 CAD 行业最难、最深的子系统之一。SolidWorks 用了数十年的 Parasolid，Autodesk 自己造的 ShapeManager，FreeCAD 用 OpenCASCADE。

不决策的代价：几何内核是核心依赖之一，必须从 Day 1 锁定，否则后续工作没基础。

## Decision

我们决定**采用 OpenCASCADE Technology (OCCT) 7.7+ 作为 myCad 的默认几何内核**。

通过 `IGeometryPort` 接口完全抽象隔离（详见 [§五 §5.1](../architecture/05-code-skeletons.md)），OCCT 类型不出 `OcctGeometryAdapter` 边界。

为长期独立性，预留自研路径文档与触发条件（详见 [§九 §9.2.1](../architecture/09-self-host-strategy.md) 与 [ADR-0009](./ADR-0009-self-host-roadmap.md)）。

## Considered Alternatives

### Option A: OpenCASCADE Technology (推荐)
- LGPL-2.1 协议
- 工业级 BRep 内核
- 覆盖 STEP/IGES、布尔、倒角、扫掠、放样等全栈
- C++ 接口（虽然风格 C++03 时代）
- 活跃维护（Open CASCADE 公司 + 社区）
- 被 FreeCAD、Salome、Gmsh 等大型项目使用

### Option B: CGAL
- 学术风格，覆盖大量计算几何算法
- 优点：算法多样，研究质量高
- 缺点：缺 STEP/IGES，无参数化特征，不适合 CAD 主线
- 适合：科学计算 / 几何处理

### Option C: Open NURBS
- 仅 NURBS 曲面
- 优点：曲面质量好
- 缺点：无 BRep 拓扑，无布尔运算 — 无法支撑 CAD

### Option D: PythonOCC
- OCCT 的 Python 包装
- 本质上还是 OCCT，且性能/集成不如直接用 OCCT

### Option E: OpenSCAD CSG
- CSG 风格（构造实体几何）
- 优点：简单
- 缺点：CSG 风格，无参数化特征树，无 STEP 互操作

### Option F: 自研 BRep 内核
- 完全可控
- 缺点：50-100 人年工程量，单人不可能
- 适合：长期愿景（详见 [§九 §9.2.1](../architecture/09-self-host-strategy.md)）

## Rationale

- **唯一可行方案**：OCCT 是开源圈唯一的工业级 BRep 内核。其他选项（CGAL / Open NURBS / OpenSCAD）都缺关键能力。自研需 50-100 人月，单人不可能。
- **协议兼容**：LGPL-2.1 与 myCad 的 LGPL-3.0 完全兼容（[ADR-0001](./ADR-0001-license-lgpl-3.md)）。
- **vcpkg 集成最佳**：OCCT 的 vcpkg portfile 是官方维护，质量高（详见 [ADR-0008](./ADR-0008-visual-studio-toolchain.md) 关于 vcpkg + Visual Studio 的论证）。
- **大型项目验证**：FreeCAD 用了 10+ 年，证明 OCCT 在生产环境可靠。
- **STEP / IGES 互操作开箱即用**：是 CAD 互操作的核心需求。

## Consequences

### Positive
- 起步即有工业级几何能力
- STEP / IGES 互操作开箱可用
- 协议无冲突
- vcpkg + Visual Studio 集成顺滑
- 大量社区资料 + 论坛支持

### Negative
- OCCT C++03 风格 API 老旧（缓解：通过 `IGeometryPort` 抽象隔离，业务层用 C++20 风格）
- OCCT 异常机制（`Standard_Failure`）需在 Adapter 内捕获并翻译为 `std::expected`
- OCCT Handle 智能指针与 `std::shared_ptr` 不兼容（缓解：Handle 不出 Adapter 层，详见 [§二 §2.6.4](../architecture/02-technical.md)）
- OCCT 编译慢（vcpkg 首次编译可达 30 min）
- LGPL-2.1 限制了"完全闭源衍生"（这是设计意图）
- 单点依赖：OCCT 停更对 myCad 有影响（缓解：[§九 §9.2.1 自研路径](../architecture/09-self-host-strategy.md)）

### Neutral
- 需维护 vcpkg.json 中的 opencascade 版本
- 需在文档/UI 中标注 "Powered by OpenCASCADE"
- 需关注 OCCT 主仓动态（季度评估一次）

## Implementation Notes

### Phase 0 必须落地

- vcpkg.json 加入 `opencascade` 依赖（锁定 7.7.x）
- `IGeometryPort` 接口设计（[§五 §5.1](../architecture/05-code-skeletons.md)）
- `OcctGeometryAdapter` 实现 `makeBox` + `tessellate` + `serialize`
- CMake 中确保 OCCT 动态链接（LGPL 边界保证）

### 隔离原则

```cpp
// ✅ 正确：Domain 用 BRepHandle（不透明 ID）
auto box = geometryPort.makeBox(10, 20, 30);

// ❌ 错误：Domain 不应见到 OCCT 类型
TopoDS_Shape shape = ...;  // 这种代码只能在 Adapter 内
```

### 可能踩的坑

| 坑 | 解决 |
|---|---|
| OCCT vcpkg 编译慢 | 用 vcpkg binary cache（GitHub Actions cache） |
| OCCT 异常逃出 Adapter | 强制 try-catch，翻译为 `std::expected` |
| OCCT Handle 误用导致内存泄漏 | Handle 不出 Adapter；BRepHandleRegistry 集中管理 |
| OCCT 大版本升级破坏 myCad | 锁定 vcpkg baseline；升级走完整回归测试 |

### 长期监控

每季度评估：
- OCCT 主仓提交频率
- OCCT 公司公告（是否有商业转向）
- OCCT 协议是否有变化
- 替代方案（CGAL、自研）的可行性变化

## References

- [OpenCASCADE 官网](https://www.opencascade.com/)
- [OCCT GitHub](https://github.com/Open-Cascade-SAS/OCCT)
- [§二 §2.6 几何内核集成](../architecture/02-technical.md)
- [§五 §5.1 IGeometryPort](../architecture/05-code-skeletons.md)
- [§九 §9.2.1 自研路径](../architecture/09-self-host-strategy.md)
- [§一 §1.4.2 OCCT 上游协作策略](../architecture/01-business.md)
- 相关 ADR: [ADR-0001](./ADR-0001-license-lgpl-3.md), [ADR-0009](./ADR-0009-self-host-roadmap.md)
