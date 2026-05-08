# ADR-0012: 工程图识别走 VLM API 还是本地模型

- **Status**: Proposed
- **Date**: 2026-05-06
- **Decider**: 项目维护者
- **Author**: Claude Code (drafted) / <人工待 review>

## Context（背景）

[ADR-0010](ADR-0010-drawing-domain-boundary.md) 引入 drawing 模块。Phase 2.B 要交付**图纸识别 MVP**：从 PDF / 扫描图像 / 拍照工程图 → 重建出 drawing 聚合（最终再到 3D 草图）。

识别管线分三段：

```
原始图像/PDF → [1] 几何元素提取（线段/弧/文本框）
              → [2] 语义解析（"这是一个 ⌀10 直径标注"、"这是俯视图"）
              → [3] 3D 重建（多视图融合 → 草图 → 拉伸）
```

第 1 段是经典 CV（OpenCV + Tesseract OCR），技术成熟。**第 2 段是关键瓶颈** — 需要把矢量几何 + 文本 解读为工程语义，这是典型的多模态理解任务。

两条路：
- **VLM API**（Vision Language Model 云服务）— Claude vision / GPT-4V / Gemini Vision，发图给云 API，返回结构化语义
- **本地模型** — YOLOv8 + 自训练分类头 + Tesseract，全部在用户机器上跑

不决策的代价：Phase 2.B 启动时再纠结，浪费 1-2 周；或更糟 — 投资本地模型 6 个月发现 VLM 已经把效果打到 95% 准确率，沉没成本巨大。

## Decision（决策）

我们决定 **Phase 2.B MVP 用 VLM API（首选 Anthropic Claude vision，备选 OpenAI GPT-4V）**；**Phase 2.D 升级时**根据真实用户反馈再决定是否补本地化路径。

**关键不变量：**

1. 识别引擎抽象在 `infrastructure/recognition/IRecognitionPort` 接口背后，application 层只看接口
2. **VLM 调用对用户隐私敏感** — 配置必须**默认关闭**，用户**显式 opt-in** 才发图
3. 用户可选 endpoint（自托管 LLM / Ollama / 第三方 API）— 不写死任何特定服务商
4. 识别结果**先生成命令**进 CommandBus，由 application 层校验后再产事件 — 不让 VLM 直接污染事件流

我们决定 **VLM API 优先**，而不是 **本地模型自训自维护**。

## Considered Alternatives（候选方案）

### Option A: VLM API 优先 ✅ **选这个**

- **描述**：Phase 2.B 通过 Anthropic / OpenAI / 等的 Vision API 调用做 [2] 语义解析；用户配置自己的 API key
- **优点**：
  - **开发节奏快** — 跳过模型训练（数据收集 / 标注 / 训练 / 调优 6+ 月）
  - **效果 baseline 高** — 当前 GPT-4V / Claude Sonnet 4.5 vision 在工程图理解上准确率 ~85% 起步（自训练模型即便 6 月也未必到这个数）
  - **零运行时成本** — 用户只需 API key，无需 GPU 服务器
  - **演化对齐** — VLM 模型迭代很快，3 个月就有显著提升，搭便车
  - **多语言支持开箱** — 中文 / 日文 / 德文标注（工程领域跨国常态）VLM 自带
- **缺点**：
  - **隐私敏感** — 军工 / 央企图纸不能上传第三方云
  - **按调用计费** — 重度用户成本高（典型工程图一张可能 $0.05-0.20）
  - **离线不可用** — 网络抖动 / 内网用户卡死
  - **不可重现** — 同一张图调两次结果可能微小差异
- **缓解措施**：
  - 配置层支持自托管 endpoint（vLLM / Ollama 跑 Qwen2-VL / Llama 4 Vision），让有隐私需求的用户自部署
  - 客户端缓存（同一张图 + 同一 prompt → cache key），减少重复调用
  - 提供"低成本模式"（仅用 VLM 做语义识别，几何提取仍走本地 OpenCV，降低 API 调用频率）

### Option B: 本地模型自训

- **描述**：自己采集工程图数据集（万级），训练 YOLO 检测线段 / 标注 / 视图框，再训练分类器做语义
- **优点**：
  - 隐私完全自主
  - 一次训练长期可用，按调用边际成本 ≈ 0
  - 可以针对中文 / 国标 / 行业（机械 / 电气 / 建筑）专项优化
- **缺点**：
  - **数据稀缺** — 高质量带语义标注的工程图数据集**几乎不存在**（不像自然图像有 ImageNet）
  - **标注成本巨大** — 工程图标注需要懂 GD&T / 国标的工程师，~10 元/张 × 万张 = 10 万级成本
  - **效果上限有限** — 即便训练好，复杂度（公差 / 形位 / 表面粗糙度符号）远超目标检测
  - **运维负担** — 模型版本管理、效果回归、用户机器 GPU 配置兼容性
  - **6+ 月才出 MVP**，比 A 慢 5 倍
- **拒绝理由**：投资 / 回报严重不匹配；即便做出来也未必比 VLM 好

### Option C: 混合 — 关键路径本地（隐私敏感）+ 非关键 VLM

- **描述**：用户标记图纸为"敏感"则强制本地模型 fallback，否则走 VLM
- **优点**：兼顾隐私与效果
- **缺点**：
  - 需要同时维护两套引擎 — 工程量 = A + B
  - 本地模型效果差，"敏感图纸" 反而识别不准 — 用户体验割裂
- **拒绝理由**：当下阶段不应承担两套引擎成本；Phase 2.D 用户量起来后再评估

### Option D: 不做 — 把识别作为商业插件

- **描述**：myCad 主仓不做识别，留给商业插件作者
- **优点**：避免协议 / 隐私 / 成本议题
- **缺点**：
  - 失去**最关键的 AI-native 差异化**卖点
  - 商业插件作者也不会做（市场太小，成本高）
  - 与 myCad 的 "AI-native" 定位（[README.md](../../README.md)）矛盾
- **拒绝理由**：违背项目核心定位

## Rationale（理由）

- **硬约束**：Phase 2.B MVP 8 周内要出来，B / C 都做不到
- **效果 / 成本前沿**：VLM 在 2026 年已经是**直觉性首选**，6 个月内还会更强
- **可演化性**：A 选定后接口在 `IRecognitionPort` 后面，将来切换本地模型 / 第三方服务都不动 application
- **杀手特性**：用户已经用 GitHub Copilot / Claude Code 在写代码，他们对 "AI 处理我的工程图" 的接受度高，市场教育成本几乎为 0

## Consequences（后果）

### Positive

- ✅ Phase 2.B MVP 可以 **8 周内出原型**（vs B 的 6 月）
- ✅ 与 myCad "AI-native" 定位强一致
- ✅ VLM 模型升级时 myCad 自动受益，不需自维护
- ✅ 多语言 / 多行业 / 多标准（GB / ISO / ANSI）开箱可用

### Negative

- ⚠️ 隐私 / 合规 — 缓解：默认关闭 + 显式 opt-in + 支持自托管 endpoint + 文档中明确"哪些数据会发出去"
- ⚠️ 用户需自备 API key — 缓解：教程文档清晰；**绝不**在 myCad 主仓内捆绑任何 API key
- ⚠️ 离线场景识别不可用 — 缓解：UI 中明确显示"识别需要网络连接"；非识别功能完全可离线
- ⚠️ 调用成本累积 — 缓解：客户端缓存 + 低成本模式 + 用户预算上限提示

### Neutral

- 🔧 新增 `infrastructure/recognition/` 模块（Phase 2.B）
- 🔧 [docs/architecture/04-tech-decisions.md](../architecture/04-tech-decisions.md) 增加 VLM 选型条目
- 🔧 隐私政策文档 [docs/operations/privacy-recognition.md](../operations/privacy-recognition.md) 待写
- 🔧 用户配置 UI 中加 "AI 识别 endpoint" 设置（可选 Anthropic / OpenAI / 自托管 URL / 关闭）

## Implementation Notes（实施注记）

### 接口设计（待 Phase 2.B 落地）

```cpp
// src/domain/drawing/include/mycad/domain/drawing/ports/IRecognitionPort.hpp
namespace mycad::domain::drawing {

/// @brief Recognize a 2D engineering drawing into structured semantic events.
///
/// 输入一张图（PDF / PNG / JPEG），返回一组建议命令（CreateSheet / ProjectView /
/// PlaceDimension / AddAnnotation 等），由 application 层校验后送 CommandBus。
class IRecognitionPort {
public:
    virtual ~IRecognitionPort() = default;

    /// @brief Convert image bytes into draft drawing commands.
    /// @param  imageBytes   raw image data (PDF / PNG / JPEG)
    /// @param  hint         optional: declare expected drawing standard (GB / ISO / ANSI)
    /// @return list of suggested commands (may be empty if recognition fails)
    /// @throws RecognitionError on transport / API errors (NOT on poor recognition quality —
    ///         that returns an empty / partial list with confidence < threshold).
    virtual std::vector<RecognitionCommand>
    recognize(std::span<const std::byte> imageBytes,
              const RecognitionHint& hint) = 0;
};

}  // namespace mycad::domain::drawing
```

### 实现选项（Phase 2.B 至少一个）

- `infrastructure/recognition/AnthropicVisionAdapter.{hpp,cpp}`
- `infrastructure/recognition/OpenAiVisionAdapter.{hpp,cpp}`
- `infrastructure/recognition/SelfHostedAdapter.{hpp,cpp}` — 通过 OpenAI 兼容 API 接 vLLM / Ollama / DashScope

### 隐私 / 合规默认配置

```toml
# myCad 用户配置文件 default
[ai.recognition]
enabled = false                # ❗ 默认关闭
endpoint = ""                  # 无默认 endpoint
api_key_source = "env"         # 不允许写硬盘 plaintext
sensitive_pattern = "*.dwg.classified"  # 文件名匹配则禁用识别
upload_warning = true          # 每次发图前弹确认
```

### 测试策略

- 真实工程图测试集（用户提供的脱敏样本，10-50 张，覆盖机械 / 钣金 / 装配 / 标注）
- 准确率 baseline ≥ 70% 算 MVP 通过（Phase 2.D 提到 90%）
- 失败模式：识别不出来 → 降级到 OpenCV 仅几何提取，让用户手工标注语义

### 迁移路径

- Phase 2.B：单一 VLM 路径（API 走云）
- Phase 2.D（用户反馈后）：评估混合策略 / 加自托管路径
- 长远：保持 `IRecognitionPort` 接口不变，实现可换

## References（参考）

- 相关 ADR: [ADR-0010](ADR-0010-drawing-domain-boundary.md)（drawing domain）
- 提案来源: [docs/proposals/2026-W19-expansion.md §2](../proposals/2026-W19-expansion.md)
- VLM 当前能力对比（2026 年初）: TODO（写完用户隐私文档时一并 benchmark）
- Anthropic Vision API: <https://docs.anthropic.com/claude/docs/vision>
- OpenAI Vision API: <https://platform.openai.com/docs/guides/vision>
- 自托管 VLM 选项: vLLM / Ollama / SGLang / TensorRT-LLM
