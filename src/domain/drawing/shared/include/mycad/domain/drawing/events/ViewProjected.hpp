#pragma once

/// @file
/// @brief ViewProjected event — a 3D model was projected onto a Sheet as a View.

namespace mycad::domain::drawing::events {

/// @brief Event: a View was projected from a 3D model onto a Sheet.
///
/// Phase 1.C：sheetId_、viewId_、sourceModelId_（AggregateId 弱引用）、
/// projectionType_、placement2D_、scale_、occurredAt_。
struct ViewProjected {
    // Sprint 1.C: 字段在此声明。
};

}  // namespace mycad::domain::drawing::events
