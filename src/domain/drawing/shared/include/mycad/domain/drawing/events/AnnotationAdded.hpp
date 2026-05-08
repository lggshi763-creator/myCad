#pragma once

/// @file
/// @brief AnnotationAdded event — a non-dimensional mark was added to a View.

namespace mycad::domain::drawing::events {

/// @brief Event: an Annotation (text / GD&T / surface roughness) was added.
///
/// Phase 1.C：viewId_、annotationId_、kind_、payload_、anchor_、occurredAt_。
struct AnnotationAdded {
    // Sprint 1.C: 字段在此声明。
};

}  // namespace mycad::domain::drawing::events
