#pragma once

/// @file
/// @brief Drawing aggregate root — a single engineering drawing artifact.
///
/// 一份工程图，作为独立聚合根存在。包含若干 Sheet（图纸），每个 Sheet 上又承载
/// View / Dimension / Annotation / BOM。Drawing 通过 AggregateId 弱引用 3D 模型，
/// 不嵌入几何（详见 ADR-0010）。

namespace mycad::domain::drawing {

/// @brief Aggregate root representing one engineering drawing artifact.
///
/// Phase 1.C 落地内容：
///   - 构造（id + 目标 3D 模型 ids）
///   - addSheet / removeSheet 操作 + 不变量校验（sheet 编号唯一）
///   - 事件流 apply / replay
///
/// 当前为占位声明，无成员。
struct Drawing {
    // Sprint 1.C: id_, sheets_, targetModels_, version_ 等字段在此声明。
};

}  // namespace mycad::domain::drawing
