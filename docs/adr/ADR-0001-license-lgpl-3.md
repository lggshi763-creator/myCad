# ADR-0001: 采用 LGPL-3.0-or-later 协议

- **Status**: Accepted
- **Date**: 2026-05-03
- **Decider**: 项目维护者
- **Author**: Claude Code (drafted) / 人工 (Accepted)

## Context

myCad 需要选定一个开源协议，作为整个项目的法律基础。协议选择影响：
- 是否允许商业插件
- 私有化部署的可行性
- 大厂"白嫖"的防护
- 与依赖（特别是 OCCT LGPL-2.1）的兼容性
- 社区接受度

不决策的代价：未明确协议的开源项目无法被任何严肃用户/客户采用。

## Decision

我们决定采用 **LGPL-3.0-or-later** 作为 myCad 的主协议。

文档与示例采用 **CC-BY 4.0**。

AI 模型权重（远期）单独评估，可能采用 Creative Commons 或自定义"商业使用需付费"协议。

## Considered Alternatives

### Option A: MIT / BSD-3
- 极宽松：任何人可闭源衍生
- 优点：社区接受度最高
- 缺点：大厂可"白嫖+换皮+营销碾压"，单人开发者无法对抗

### Option B: LGPL-3.0-or-later（推荐）
- 弱传染：仅修改内核需开源
- 动态链接的应用可以闭源（商业插件友好）
- 与 OCCT LGPL-2.1 兼容
- 社区接受度中高（CAD 圈认可）

### Option C: GPL-3.0
- 强传染：衍生作品必须开源
- 优点：最强大厂防护
- 缺点：商业插件几乎不可能（扼杀生态）

### Option D: AGPL-3.0
- 极强传染：含网络服务
- 优点：连云服务都纳入传染
- 缺点：企业禁用，私有化部署客户无法接受

### Option E: 双协议（GPL + 商业版）
- 内核 GPL，商业版另购
- 优点：高客单价
- 缺点：需要 CLA，对早期个人项目极大法律和管理负担，劝退社区贡献者

## Rationale

- **LGPL 与 OCCT 兼容**：myCad 强依赖 OCCT，OCCT 是 LGPL-2.1。选 LGPL-3.0 是最自然的兼容选择，避免协议冲突的法律风险。
- **允许商业插件存在**：第三方可以开发闭源商业插件，通过动态链接调用 myCad API。这是构建"插件市场"商业生态的前提（详见 [§一 §1.3 商业模式](../architecture/01-business.md)）。
- **保护内核不被闭源 fork**：任何人修改 myCad 内核源码必须开源回馈，防止大厂直接拿走改名上线。
- **私有化部署友好**：企业客户部署 myCad 服务端不会触发协议传染（与 AGPL 关键区别）。
- **不选 GPL v3**：GPL 会扼杀商业插件生态，而插件生态正是商业模式核心。
- **不选 MIT**：MIT 太宽松，大厂可以拿去改名做闭源产品，单人开发者无法对抗。
- **不做双协议**：双协议商业版需要 CLA，对早期个人项目是极大负担，会劝退社区贡献者。

## Consequences

### Positive
- 与 OCCT 等核心依赖协议兼容
- 商业插件生态可以建立（动态链接闭源插件合规）
- 企业私有化部署不触发传染
- 大厂"换皮上线"被阻止
- 社区接受度高

### Negative
- 静态链接 myCad 的应用必须 LGPL → 限制了某些"嵌入式"用法
- 需要全员（贡献者）理解 LGPL 边界（缓解：CONTRIBUTING.md 中说明）
- 部分极端商业场景（如完全闭源 CAD 衍生产品）不可行（这正是设计意图）

### Neutral
- 必须维护 LICENSE 文件 + COPYING（GPL-3.0 文本）
- 每个源文件建议加 SPDX 头
- 第三方依赖必须协议兼容审核（详见 [§四 4.2 协议矩阵](../architecture/04-tech-decisions.md)）

## Implementation Notes

### Phase 0 必须落地

- `LICENSE` 文件（含 LGPL-3.0 全文 + 短通知）
- 每个源文件头部：
  ```cpp
  // SPDX-License-Identifier: LGPL-3.0-or-later
  // Copyright (C) 2026 myCad contributors
  ```
- `CONTRIBUTING.md` 写明：贡献即同意 LGPL-3.0-or-later
- `README.md` 顶部 license badge
- 第三方依赖协议审核脚本（远期 CI）

### Domain 隔离意义

为了让 LGPL 边界清晰，Domain 层零外部依赖（[ADR-0002](./ADR-0002-domain-zero-deps.md)）使得：
- Domain 层是纯 LGPL myCad 代码
- Adapter 与 OCCT 的 LGPL 边界清晰
- 商业插件通过 myCad Plugin API 与内核动态链接，自然合规

## References

- [GNU LGPL-3.0 全文](https://www.gnu.org/licenses/lgpl-3.0.txt)
- [GNU GPL FAQ](https://www.gnu.org/licenses/gpl-faq.html)
- [§一 §1.3.1 开源协议选型](../architecture/01-business.md)
- [§七 §7.2 LGPL 商业化风险](../architecture/07-risks.md)
- 相关 ADR: [ADR-0002](./ADR-0002-domain-zero-deps.md)（Domain 零依赖巩固协议边界）
