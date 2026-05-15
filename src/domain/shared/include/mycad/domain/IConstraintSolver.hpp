#pragma once

#include <array>
#include <cstdint>
#include <span>

/// @file Domain port for geometric constraint solving (sketch Phase 1.A).

namespace mycad::domain {

// ---------------------------------------------------------------------------
// Solver vocabulary types
// ---------------------------------------------------------------------------

/// @brief A scalar degree of freedom in the constraint system.
struct SolverVariable {
    std::uint32_t id{0};
    double value{0.0};
    bool fixed{false};  ///< Fixed variables are not modified by the solver.
};

/// @brief Geometric constraint kinds supported by the solver.
enum class ConstraintKind : std::uint8_t {
    Distance,       ///< |p1 - p2| == targetValue
    Angle,          ///< signed angle (from, vertex, to) == targetValue (radians)
    Coincident,     ///< p1 == p2  (targetValue ignored)
    Parallel,       ///< lines (p1,p2) and (p3,p4) are parallel (targetValue ignored)
    Perpendicular,  ///< lines (p1,p2) and (p3,p4) are perpendicular (targetValue ignored)
    Horizontal,     ///< p1.y == p2.y  (targetValue ignored)
    Vertical,       ///< p1.x == p2.x  (targetValue ignored)
};

/// @brief A single geometric constraint referencing up to 4 variable IDs.
///
/// Variable layout per ConstraintKind:
/// - Distance / Coincident / Horizontal / Vertical: varIds[0..1] = (x1,y1), (x2,y2)
/// - Angle: varIds = (xFrom, yFrom, xVertex, yVertex); separate Angle constraint for (to)
/// - Parallel / Perpendicular: varIds = (x1,y1,x2,y2) for line1; second constraint for line2
///
/// 🗒  Full constraint algebra is elaborated in Sprint 1.A; this is the Phase 0 skeleton.
struct SolverConstraint {
    std::uint32_t id{0};
    ConstraintKind kind{ConstraintKind::Distance};
    std::array<uint32_t, 4> varIds{};
    double targetValue{0.0};
};

/// @brief Outcome of a single solve call.
struct SolveResult {
    enum class Status : std::uint8_t {
        Solved,           ///< All constraints satisfied within tolerance.
        Underdetermined,  ///< Fewer constraints than DOF; solution may not be unique.
        Overdetermined,   ///< Conflicting constraints; no exact solution.
        Failed,           ///< Solver did not converge.
    };

    Status status{Status::Failed};
    double residual{0.0};  ///< Max constraint violation in the final state.
    int iterations{0};     ///< Number of solver iterations consumed.
};

// ---------------------------------------------------------------------------
// Port interface
// ---------------------------------------------------------------------------

/// @brief Port that drives an external constraint solver for the sketch domain.
///
/// 调用方传入可变 SolverVariable 列表和约束列表；
/// 求解成功后 variables 的 value 字段被就地更新。
///
/// @thread-safe 实现类应保证每次 solve() 调用线程安全（无共享可变状态）。
class IConstraintSolver {
public:
    virtual ~IConstraintSolver() = default;

    IConstraintSolver(const IConstraintSolver&) = delete;
    IConstraintSolver& operator=(const IConstraintSolver&) = delete;

    /// @brief Solves the constraint system, updating variable values in place.
    ///
    /// @param variables  Variables to solve; fixed==true ones are left untouched.
    /// @param constraints Constraints the solution must satisfy.
    /// @return SolveResult with status, residual, and iteration count.
    /// @noexcept-ok
    [[nodiscard]] virtual SolveResult
    solve(std::span<SolverVariable> variables,
          std::span<const SolverConstraint> constraints) noexcept = 0;

protected:
    IConstraintSolver() = default;
};

}  // namespace mycad::domain
