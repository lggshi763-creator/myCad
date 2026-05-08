#pragma once

/// @file
/// @brief Sheet — one page within a Drawing, with frame and title block.
///
/// 一份 Drawing 内的单页图纸：含图框规格（A0/A1/A2/A3/A4 或自定义）、
/// 标题栏、单位制（mm / inch）、绘图比例。一个 sheet 可承载多个 view。

namespace mycad::domain::drawing {

/// @brief A single page within a Drawing — frame + title block + scale + views.
///
/// Phase 1.C：完整字段（id_、drawingRef_、frameSize_、titleBlock_、scale_、unit_）
/// 与不变量（sheet 内 view 不能空间重叠）。
struct Sheet {
    // Sprint 1.C: 字段在此声明。
};

}  // namespace mycad::domain::drawing
