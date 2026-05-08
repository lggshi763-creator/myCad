#pragma once

/// @file
/// @brief IDrawingProjectionPort — port for 3D-to-2D projection (HLR).
///
/// Domain 不依赖 OCCT，但需要"把一个 3D 模型按 projectionType 投影成一组 2D
/// 边线 + 隐藏线"的能力。infrastructure/drawing/OcctProjectionAdapter 实现此端口，
/// 内部走 OCCT 的 HLRBRep 模块。

namespace mycad::domain::drawing {

/// @brief Port that converts a 3D model into a 2D projection (visible + hidden lines).
///
/// Phase 1.C：定义完整接口，参数为 ViewSpec（含 projectionType / sourceModelId）+
/// 返回 ViewGeometry（visibleEdges / hiddenEdges / boundaries）。当前为占位声明。
class IDrawingProjectionPort {
public:
    virtual ~IDrawingProjectionPort() = default;
    // Sprint 1.C: project(spec, model) -> ViewGeometry
};

}  // namespace mycad::domain::drawing
