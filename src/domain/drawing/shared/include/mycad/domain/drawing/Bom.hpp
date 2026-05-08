#pragma once

/// @file
/// @brief Bom — bill of materials, auto-generated from an assembly tree.
///
/// 明细栏：装配体的 BOM 表。从装配树自动抽取，包含件号、零件名、数量、
/// 材料、备注。一个 sheet 可挂一份 BOM；BOM 行的更新由 BomGenerated 事件触发，
/// 用户可手工覆盖（覆盖也是事件）。

namespace mycad::domain::drawing {

/// @brief Bill of materials — assembly's parts/quantities table on a Sheet.
///
/// Phase 1.E：rows_、sourceAssemblyId_、columns_（用户可定制列）。
struct Bom {
    // Sprint 1.E: 字段在此声明。
};

}  // namespace mycad::domain::drawing
