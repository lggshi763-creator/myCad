# ADR-0005: OpenGL 4.5 而非 Vulkan

- **Status**: Accepted
- **Date**: 2026-05-03
- **Decider**: 项目维护者
- **Author**: Claude Code (drafted) / 人工 (Accepted)

## Context

myCad 需要 3D 渲染能力。需要选择底层图形 API：
- 影响代码量、维护成本、性能上限
- 影响跨平台支持（Windows/Linux/macOS/Web）
- 影响个人开发者的可控性

不决策的代价：渲染层是 Phase 0 必须落地的核心子系统。

## Decision

我们决定**采用 OpenGL 4.5 Core Profile + DSA（Direct State Access）作为 myCad 的默认渲染 API**。

通过 `IRenderPort` 接口抽象，远期可加 wgpu 后端（覆盖 macOS Metal + Web WASM）。详见 [§九 §9.2.5](../architecture/09-self-host-strategy.md)。

## Considered Alternatives

### Option A: OpenGL 4.5 Core + DSA (推荐)
- 跨平台（Windows / Linux / macOS / Android / Web via WebGL2）
- 极成熟，驱动稳定
- 代码量适中
- DSA 让代码组织干净
- 缺点：苹果已弃用，macOS 长期需迁移

### Option B: Vulkan
- 多线程渲染原生支持
- 性能上限高
- 缺点：代码量是 GL 的 3-5 倍
- 缺点：同步原语易写错
- 缺点：CAD 行业几乎无采用
- 缺点：CAD 渲染瓶颈不在 draw call

### Option C: Direct3D 11/12
- Windows 性能最佳
- 缺点：完全不跨平台
- 缺点：开发体验受 Microsoft 工具链绑定

### Option D: Metal
- macOS 性能最佳
- 缺点：仅 macOS

### Option E: wgpu (跨平台后端)
- 跨 Vulkan / Metal / D3D12 的统一抽象
- 优点：未来友好（含 Web）
- 缺点：年轻（1.0 未发布），生态薄
- 缺点：C++ 绑定（wgpu-native）需要额外依赖

## Rationale

**核心论据：CAD 渲染瓶颈不在 draw call，而在几何与拓扑**

- CAD 典型场景每帧 draw call ≈ 几百到几千个，远低于游戏（数万）
- 真正的瓶颈在：
  - BRep → Mesh 化（CPU 端，OCCT BRepMesh）
  - 大型装配体的 LOD 与剔除（CPU 端）
  - 拓扑选择（CPU 端 ray cast）
- Vulkan 的"减少驱动开销"对 CAD 不重要

**Vulkan 的隐藏成本**

- shader 必须用 SPIR-V 或自己编译 GLSL
- 同步原语（Semaphore / Fence / Barrier）极易写错
- 验证层调试经验门槛高
- 个人开发者投入产出比极低

**OpenGL 4.5 + DSA 的优势**

- DSA 让代码组织干净（无需 bind state 模式）
- 所有现代特性可用（Compute Shader、SSBO、Indirect Drawing、Bindless Texture）
- Visual Studio 调试器对 OpenGL 支持良好
- OCCT 自带 OpenGL 渲染（虽然我们自建，但参考资料丰富）

**长期演进路径**

- 通过 `IRenderPort` 抽象封装
- macOS 长期问题通过引入 wgpu 后端解决（Phase 3 评估）
- Web 端未来通过同一份 wgpu 后端覆盖

## Consequences

### Positive
- 起步快：Phase 0 渲染基础 1 个 sprint 可就绪
- 三平台覆盖（Windows + Linux + macOS）
- 大量 OpenGL 教程 / Stack Overflow 资料
- Visual Studio + RenderDoc 调试体验良好
- 通过 Adapter 封装，未来可平滑过渡 wgpu

### Negative
- macOS 长期可用性下降（Apple 已弃用 OpenGL，仍支持但无新特性）
- 不能享受 Vulkan 的多线程渲染（缓解：CAD 不需要）
- 部分高级特性（Mesh Shader、Ray Tracing）OpenGL 不支持（缓解：CAD 不需要）

### Neutral
- 需维护 GLSL shader（远期考虑用 shaderc 编译时转换为 SPIR-V）
- macOS 用旧 GL 上下文（4.1）— 部分 4.5 特性需 fallback
- 需配置 vcpkg 中的 glew/gl3w/glad 选其一作为 loader

## Implementation Notes

### Phase 0 必须落地

- `IRenderPort` 接口设计
- `OpenGLRenderAdapter` 实现：基础渲染三角网格 + 简单 PBR
- Camera（perspective + orbit control）
- 嵌入 `QOpenGLWidget`

### 推荐技术栈

- **OpenGL Loader**：glad（vcpkg 提供）
- **Shader 管理**：自家管理（远期 shaderc 转 SPIR-V）
- **Math**：复用 Eigen（避免引入 glm）
- **调试**：RenderDoc（VS 中 Debug → Performance Profiler → GPU Usage 也支持）

### macOS 兼容性策略

```cmake
if(APPLE)
    # macOS 仅支持 OpenGL 4.1 Core
    target_compile_definitions(mycad_render PRIVATE MYCAD_GL_VERSION=410)
else()
    target_compile_definitions(mycad_render PRIVATE MYCAD_GL_VERSION=450)
endif()
```

部分 4.5 特性（如 DSA）在 macOS fallback 到传统 bind state 风格。

### 长期演进触发条件

- macOS 真正不可用（Apple 移除 OpenGL 支持）→ 引入 wgpu Adapter
- Web 端用户需求出现 → 引入 wgpu Adapter（同一后端）
- 大型装配体（10+ 万实体）性能瓶颈 → 评估 Vulkan multi-threaded rendering（次优先级）

详见 [§九 §9.2.5](../architecture/09-self-host-strategy.md)。

## References

- [§二 §2.7 渲染架构设计](../architecture/02-technical.md)
- [§九 §9.2.5 OpenGL 自研/迁移路径](../architecture/09-self-host-strategy.md)
- [OpenGL DSA 教程](https://www.khronos.org/opengl/wiki/Direct_State_Access)
- [Apple OpenGL 弃用通告](https://developer.apple.com/macos/whats-new/)
- 相关 ADR: 无直接相关 ADR
