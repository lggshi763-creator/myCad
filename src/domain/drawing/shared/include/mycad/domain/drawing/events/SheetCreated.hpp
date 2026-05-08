#pragma once

/// @file
/// @brief SheetCreated event — a new Sheet was added to a Drawing.

namespace mycad::domain::drawing::events {

/// @brief Event: a new Sheet was created within a Drawing.
///
/// Phase 1.C：drawingId_、sheetId_、frameSize_、scale_、unit_、occurredAt_。
/// 不可变，遵循 ADR-0003。
struct SheetCreated {
    // Sprint 1.C: 字段在此声明。
};

}  // namespace mycad::domain::drawing::events
