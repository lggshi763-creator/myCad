#pragma once

/// @file
/// @brief View — a 2D projection of a 3D model placed on a Sheet.
///
/// 视图：3D 模型的某个 2D 投影。类型涵盖正视图（Front / Top / Right）、
/// 等轴测、剖视图、局部放大。视图引用 3D 模型 AggregateId，不嵌入几何。

namespace mycad::domain::drawing {

/// @brief A 2D projection of a 3D model as a placed entity on a Sheet.
///
/// Phase 1.C：projectionType_ 枚举（Front/Top/Right/Iso/Section/Detail）、
/// sourceModelId_、placement2D_、scale_。
struct View {
    // Sprint 1.C: 字段在此声明。
};

}  // namespace mycad::domain::drawing
