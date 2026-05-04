# ADR-0006: 插件双 ABI 边界（C 入口 + C++ Host）

- **Status**: Accepted
- **Date**: 2026-05-03
- **Decider**: 项目维护者
- **Author**: Claude Code (drafted) / 人工 (Accepted)

## Context

myCad 是微内核插件架构（详见 [§二 §2.5](../architecture/02-technical.md)）。插件作为独立的动态库（`.dll` / `.so` / `.dylib`）由 myCad 主程序加载。

C++ 没有稳定 ABI（应用二进制接口）。同一份头文件用不同编译器、不同 STL 实现、不同优化等级编译出的二进制不兼容。这意味着：
- myCad 升级 → 老插件二进制崩溃
- 插件作者必须用与 myCad 完全一致的编译环境
- 任何 STL 类型改动都可能破坏插件

CAD 商业模式核心是"插件市场" — 第三方开发者依赖插件接口稳定性才会投资。

不决策的代价：没有清晰 ABI 策略 → 插件生态无法建立。

## Decision

我们决定采用**双 ABI 边界**设计：

**Layer 1：核心 ABI（C 风格，永远稳定）**
- 插件入口函数：`mycad_createPlugin` / `mycad_destroyPlugin` / `mycad_pluginAbiVersion`
- 全部 `extern "C"` + 仅传 POD 类型 + 整数版本号
- 主程序加载插件时严格校验版本号匹配

**Layer 2：丰富 API（C++ 风格，按主版本兼容）**
- 通过 `IPluginHost` C++ 接口提供能力
- 仅承诺**同一主版本号下 ABI 兼容**
- 主版本升级（1.x → 2.x）需要插件重新编译
- 插件作者必须用项目指定的 MSVC 版本编译（Visual Studio 2022 17.x）

## Considered Alternatives

### Option A: 双 ABI 边界（推荐）
- 优点：核心入口永远稳定 + C++ API 提供丰富能力
- 缺点：架构略复杂

### Option B: 纯 C ABI
- 全部接口用 C 风格
- 优点：永远稳定
- 缺点：插件作者写 C++ 时大量手动翻译；C++ 类不能跨边界 → 插件能力受限

### Option C: 纯 C++ ABI
- 一律 C++ 接口
- 优点：开发体验最好
- 缺点：任何 STL 改动都破坏 ABI；插件作者必须严格匹配编译环境

### Option D: 进程隔离（subprocess plugins）
- 插件作为独立进程，通过 IPC 通信
- 优点：完全隔离，编译器无关
- 缺点：CAD 场景下 IPC 开销过大（几何对象传递昂贵）

### Option E: WebAssembly 沙箱
- 插件编译为 WASM，运行在沙箱
- 优点：完全跨编译器 + 安全
- 缺点：技术不成熟（C++ 编译到 WASM 大；性能损失；调试困难）
- 适合：远期 P2（Phase 3+）

## Rationale

- **CAD 插件需要 C++ 能力**：纯 C ABI 让插件作者无法用 OOP / RAII / STL，开发体验灾难
- **核心入口 = 长期承诺**：版本检查 + 创建/销毁是最少必须，C 风格让它永远稳定
- **同主版本 C++ ABI 是现实可控**：用同一个 MSVC（Visual Studio 2022）编译所有插件，ABI 兼容是可保证的
- **不引入进程隔离的开销**：CAD 几何对象传递频繁，IPC 模型不可接受
- **WASM 沙箱留远期**：技术不成熟，但作为长期方向预留

## Consequences

### Positive
- 插件入口永远稳定（C ABI）
- 插件开发体验好（C++ 接口，能用 STL / RAII / 模板）
- ABI 不匹配可早期检测（版本号校验）
- 商业插件可与 myCad 主程序版本独立分发
- LGPL 边界自然（动态链接）

### Negative
- 主版本升级（1.x → 2.x）破坏所有插件二进制（缓解：主版本承诺 2 年内不破坏）
- 插件作者必须用项目指定的编译器（缓解：CMake template 让用户开箱即用）
- 双层架构需要额外设计与文档（缓解：清晰文档 + 模板代码）

### Neutral
- 需维护 `kCorePluginAbiVersion` 常量（仅在核心入口变更时升级）
- 需维护"主版本兼容矩阵"（哪些插件版本兼容哪些 myCad 主版本）
- 插件 SDK 需作为单独 release artifact 分发

## Implementation Notes

### 核心 C ABI（永远稳定）

```cpp
inline constexpr std::uint32_t kCorePluginAbiVersion = 1;

#if defined(_WIN32)
    #define MYCAD_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
    #define MYCAD_PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif

#define MYCAD_PLUGIN_ENTRY(PluginClass)                                       \
    MYCAD_PLUGIN_EXPORT mycad::plugin::IPlugin* mycad_createPlugin() {        \
        return new PluginClass();                                             \
    }                                                                         \
    MYCAD_PLUGIN_EXPORT void mycad_destroyPlugin(mycad::plugin::IPlugin* p) { \
        delete p;                                                             \
    }                                                                         \
    MYCAD_PLUGIN_EXPORT std::uint32_t mycad_pluginAbiVersion() {              \
        return mycad::plugin::kCorePluginAbiVersion;                          \
    }
```

### 主程序加载流程

```cpp
auto handle = LoadLibrary("foo.dll");
auto getAbiVer = (uint32_t(*)())GetProcAddress(handle, "mycad_pluginAbiVersion");
if (getAbiVer() != kCorePluginAbiVersion) {
    // 拒绝加载，报告 AbiMismatch 错误
    return std::unexpected(PluginError{PluginErrorKind::AbiMismatch, ...});
}
auto create = (IPlugin*(*)())GetProcAddress(handle, "mycad_createPlugin");
auto* plugin = create();
// 现在可以调用 plugin->onLoad(host) 等 C++ 方法
```

### C++ API 兼容性策略

主版本号编码到 namespace：

```cpp
namespace mycad::host_v1 {
    class IPluginHost { /* ... */ };
}

// 主版本升级时：
namespace mycad::host_v2 {
    class IPluginHost { /* ... */ };
}
// 旧插件用 host_v1::IPluginHost*，主程序提供 v1 兼容包装
```

### 编译器要求文档

`CONTRIBUTING.md` 与 `docs/architecture/08-vs-toolchain.md` 明确：
- 插件必须用 MSVC v143（Visual Studio 2022）编译
- C++ 标准：C++20
- vcpkg 依赖版本与 myCad 主程序一致

### Phase 0 必须落地

- `IPlugin` + `IPluginHost` 接口（[§五 §5.4](../architecture/05-code-skeletons.md)）
- `MYCAD_PLUGIN_ENTRY` 宏
- ABI 版本号常量
- `PluginRegistry` 加载逻辑（含 ABI 校验）
- `tools/plugin-template/` — 第三方插件 5 分钟跑通的 VS 模板（详见 [§八 §8.8](../architecture/08-vs-toolchain.md)）

## References

- [§二 §2.5 微内核插件总线](../architecture/02-technical.md)
- [§五 §5.4 IPlugin 骨架](../architecture/05-code-skeletons.md)
- [§八 §8.8 插件开发体验](../architecture/08-vs-toolchain.md)
- C++ ABI 兼容性：https://itanium-cxx-abi.github.io/cxx-abi/
- COM 设计哲学：Microsoft COM Programmer's Reference
- 相关 ADR: [ADR-0008](./ADR-0008-visual-studio-toolchain.md)（统一编译器是 ABI 兼容的前提）
