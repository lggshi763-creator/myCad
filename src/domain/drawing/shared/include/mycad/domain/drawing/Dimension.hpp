#pragma once

/// @file
/// @brief Dimension — a measurement annotation on a View.
///
/// 尺寸标注：线性距离、角度、半径、直径四种。每个标注引用 View 上某些
/// 几何元素的 ID（点、线段、弧），并指定标注线的位置。

namespace mycad::domain::drawing {

/// @brief A dimensional annotation (linear / angular / radius / diameter).
///
/// Phase 1.C：DimensionType 枚举、targetGeometryRefs_、placement_、tolerance_。
struct Dimension {
    // Sprint 1.C: 字段在此声明。
};

}  // namespace mycad::domain::drawing
