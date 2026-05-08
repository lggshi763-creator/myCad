#pragma once

/// @file
/// @brief IDrawingExportPort — port for exporting Drawing to DXF / PDF.
///
/// Domain 不依赖 libdxfrw / Qt，但需要"把 Drawing 聚合写出为 DXF / PDF"的能力。
/// infrastructure/drawing/DxfWriter 与 PdfRenderer 实现此端口（详见 ADR-0011）。

namespace mycad::domain::drawing {

/// @brief Port that serializes a Drawing aggregate to a target format.
///
/// Phase 1.C：定义完整接口，含 exportToDxf / exportToPdf 两个方法。
/// 输入是 Drawing 聚合的快照，输出是字节流（写文件由 caller 决定）。
/// 当前为占位声明。
class IDrawingExportPort {
public:
    virtual ~IDrawingExportPort() = default;
    // Sprint 1.C: exportToDxf(drawing) -> bytes
    // Sprint 1.C: exportToPdf(drawing) -> bytes
};

}  // namespace mycad::domain::drawing
