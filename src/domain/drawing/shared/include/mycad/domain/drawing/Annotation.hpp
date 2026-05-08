#pragma once

/// @file
/// @brief Annotation — non-dimensional marks: text, GD&T, surface roughness.
///
/// 注释：非尺寸的图纸标注。文字注释、形位公差符号（平行度/垂直度/位置度等）、
/// 表面粗糙度符号、焊接符号、引线注释。与 Dimension 区分：Annotation 不参与
/// 测量验证，仅作信息呈现。

namespace mycad::domain::drawing {

/// @brief A non-dimensional annotation on a View (text / GD&T / surface).
///
/// Phase 1.C：AnnotationKind 枚举、payload_、anchor_。
struct Annotation {
    // Sprint 1.C: 字段在此声明。
};

}  // namespace mycad::domain::drawing
