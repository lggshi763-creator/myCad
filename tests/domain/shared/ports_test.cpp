#include <catch2/catch_test_macros.hpp>
#include <mycad/domain/IConstraintSolver.hpp>
#include <mycad/domain/IEntityRegistry.hpp>
#include <mycad/domain/IEventStore.hpp>

#include <unordered_map>
#include <unordered_set>

namespace mycad::domain::tests {

// ===========================================================================
// IEventStore stub
// ===========================================================================

/// In-memory event store stub.  Stores raw pointers — test lifetime managed
/// by the test case.  NOT suitable for production.
class StubEventStore final : public IEventStore {
public:
    void
    append(AggregateId id, Version expected, std::span<const DomainEvent* const> events) override {
        auto& stream = streams_[id];
        if (stream.version != expected) {
            throw ConcurrencyError(id, expected, stream.version);
        }
        for (const DomainEvent* ev : events) {
            stream.ptrs.push_back(ev);
            stream.version = ev->aggregateVersion();
        }
    }

    std::vector<std::unique_ptr<DomainEvent>> load(AggregateId id) override {
        // Stub returns empty — real impl reconstructs via EventTypeRegistry.
        (void)id;
        return {};
    }

    std::vector<std::unique_ptr<DomainEvent>> loadSince(AggregateId id, Version from) override {
        (void)id;
        (void)from;
        return {};
    }

    Version latestVersion(AggregateId id) override {
        const auto it = streams_.find(id);
        return it != streams_.end() ? it->second.version : Version{0};
    }

    std::size_t eventCount(AggregateId id) const {
        const auto it = streams_.find(id);
        return it != streams_.end() ? it->second.ptrs.size() : 0;
    }

private:
    struct Stream {
        Version version{0};
        std::vector<const DomainEvent*> ptrs;
    };

    struct AggregateIdHash {
        std::size_t operator()(AggregateId id) const noexcept {
            return std::hash<AggregateId>{}(id);
        }
    };

    std::unordered_map<AggregateId, Stream, AggregateIdHash> streams_;
};

// Minimal concrete event for store tests
class ThingHappened final : public DomainEvent {
    MYCAD_DOMAIN_EVENT("mycad.test.ThingHappened")
public:
    explicit ThingHappened(AggregateId id, Version ver, std::uint64_t ts)
        : DomainEvent(id, ver, ts) {}
};

// ===========================================================================
// IEventStore tests
// ===========================================================================

TEST_CASE("IEventStore -latestVersion returns v0 for unknown aggregate",
          "[domain][shared][store]") {
    StubEventStore store;
    REQUIRE(store.latestVersion(AggregateId{}).value == 0);
}

TEST_CASE("IEventStore -append single event advances version", "[domain][shared][store]") {
    StubEventStore store;
    AggregateId aid;
    const ThingHappened ev{aid, Version{1}, 0};
    const DomainEvent* ptr = &ev;
    store.append(aid, Version{0}, std::span<const DomainEvent* const>{&ptr, 1});
    REQUIRE(store.latestVersion(aid).value == 1);
    REQUIRE(store.eventCount(aid) == 1);
}

TEST_CASE("IEventStore -append two events in sequence", "[domain][shared][store]") {
    StubEventStore store;
    AggregateId aid;
    const ThingHappened e1{aid, Version{1}, 0};
    const ThingHappened e2{aid, Version{2}, 1};
    const DomainEvent* p1 = &e1;
    const DomainEvent* p2 = &e2;
    store.append(aid, Version{0}, std::span<const DomainEvent* const>{&p1, 1});
    store.append(aid, Version{1}, std::span<const DomainEvent* const>{&p2, 1});
    REQUIRE(store.latestVersion(aid).value == 2);
    REQUIRE(store.eventCount(aid) == 2);
}

TEST_CASE("IEventStore -append with wrong expected version throws ConcurrencyError",
          "[domain][shared][store]") {
    StubEventStore store;
    AggregateId aid;
    const ThingHappened ev{aid, Version{1}, 0};
    const DomainEvent* ptr = &ev;
    store.append(aid, Version{0}, std::span<const DomainEvent* const>{&ptr, 1});
    // Now try to append with stale expected version{0} again
    const ThingHappened ev2{aid, Version{2}, 0};
    const DomainEvent* ptr2 = &ev2;
    REQUIRE_THROWS_AS(store.append(aid, Version{0}, std::span<const DomainEvent* const>{&ptr2, 1}),
                      ConcurrencyError);
}

TEST_CASE("IEventStore -ConcurrencyError carries expected and actual version",
          "[domain][shared][store]") {
    StubEventStore store;
    AggregateId aid;
    const ThingHappened ev{aid, Version{1}, 0};
    const DomainEvent* ptr = &ev;
    store.append(aid, Version{0}, std::span<const DomainEvent* const>{&ptr, 1});

    try {
        const ThingHappened ev2{aid, Version{2}, 0};
        const DomainEvent* ptr2 = &ev2;
        store.append(aid, Version{0}, std::span<const DomainEvent* const>{&ptr2, 1});
        FAIL("Expected ConcurrencyError");
    } catch (const ConcurrencyError& e) {
        REQUIRE(e.expected().value == 0);
        REQUIRE(e.actual().value == 1);
    }
}

TEST_CASE("IEventStore -load returns empty vector (stub)", "[domain][shared][store]") {
    StubEventStore store;
    const auto events = store.load(AggregateId{});
    REQUIRE(events.empty());
}

// ===========================================================================
// IConstraintSolver stub
// ===========================================================================

/// Stub solver: marks all variables as their current value, returns Solved.
class StubConstraintSolver final : public IConstraintSolver {
public:
    SolveResult solve(std::span<SolverVariable> vars,
                      std::span<const SolverConstraint> /*constraints*/) noexcept override {
        // Trivial: satisfy a single Distance constraint if present.
        // For testing we just return Solved with zero residual.
        (void)vars;
        lastCallVarCount_ = static_cast<int>(vars.size());
        return SolveResult{SolveResult::Status::Solved, 0.0, 1};
    }

    int lastCallVarCount() const noexcept {
        return lastCallVarCount_;
    }

private:
    int lastCallVarCount_{0};
};

// ===========================================================================
// IConstraintSolver tests
// ===========================================================================

TEST_CASE("IConstraintSolver -solve with no constraints returns Solved",
          "[domain][shared][solver]") {
    StubConstraintSolver s;
    std::vector<SolverVariable> vars{{0, 1.0}, {1, 2.0}};
    const SolveResult r = s.solve(vars, {});
    REQUIRE(r.status == SolveResult::Status::Solved);
    REQUIRE(r.iterations == 1);
}

TEST_CASE("IConstraintSolver -solve receives correct variable count", "[domain][shared][solver]") {
    StubConstraintSolver s;
    std::vector<SolverVariable> vars(5);
    s.solve(vars, {});
    REQUIRE(s.lastCallVarCount() == 5);
}

TEST_CASE("IConstraintSolver -SolverVariable fixed flag is preserved", "[domain][shared][solver]") {
    SolverVariable v{1, 3.14, true};
    REQUIRE(v.fixed);
    REQUIRE(v.id == 1);
}

TEST_CASE("IConstraintSolver -SolveResult default status is Failed", "[domain][shared][solver]") {
    const SolveResult r;
    REQUIRE(r.status == SolveResult::Status::Failed);
    REQUIRE(r.residual == 0.0);
    REQUIRE(r.iterations == 0);
}

TEST_CASE("IConstraintSolver -ConstraintKind values are distinct", "[domain][shared][solver]") {
    REQUIRE(ConstraintKind::Distance != ConstraintKind::Angle);
    REQUIRE(ConstraintKind::Coincident != ConstraintKind::Parallel);
    REQUIRE(ConstraintKind::Horizontal != ConstraintKind::Vertical);
}

// ===========================================================================
// IEntityRegistry stub
// ===========================================================================

class StubEntityRegistry final : public IEntityRegistry {
public:
    EntityId create() override {
        const EntityId id = nextId_++;
        alive_.insert(id);
        return id;
    }

    void destroy(EntityId id) noexcept override {
        alive_.erase(id);
    }

    bool isAlive(EntityId id) const noexcept override {
        return alive_.count(id) > 0;
    }

    std::size_t size() const noexcept override {
        return alive_.size();
    }

    void clear() noexcept override {
        alive_.clear();
    }

private:
    EntityId nextId_{0};
    std::unordered_set<EntityId> alive_;
};

// ===========================================================================
// IEntityRegistry tests
// ===========================================================================

TEST_CASE("IEntityRegistry -new registry has size 0", "[domain][shared][ecs]") {
    StubEntityRegistry reg;
    REQUIRE(reg.size() == 0);
}

TEST_CASE("IEntityRegistry -create returns unique ids", "[domain][shared][ecs]") {
    StubEntityRegistry reg;
    const EntityId a = reg.create();
    const EntityId b = reg.create();
    REQUIRE(a != b);
}

TEST_CASE("IEntityRegistry -created entity is alive", "[domain][shared][ecs]") {
    StubEntityRegistry reg;
    const EntityId id = reg.create();
    REQUIRE(reg.isAlive(id));
}

TEST_CASE("IEntityRegistry -destroyed entity is no longer alive", "[domain][shared][ecs]") {
    StubEntityRegistry reg;
    const EntityId id = reg.create();
    reg.destroy(id);
    REQUIRE_FALSE(reg.isAlive(id));
}

TEST_CASE("IEntityRegistry -destroy kNullEntity is a no-op", "[domain][shared][ecs]") {
    StubEntityRegistry reg;
    reg.create();
    reg.destroy(kNullEntity);  // must not throw or crash
    REQUIRE(reg.size() == 1);
}

TEST_CASE("IEntityRegistry -size tracks live entities", "[domain][shared][ecs]") {
    StubEntityRegistry reg;
    const EntityId a = reg.create();
    const EntityId b = reg.create();
    reg.create();
    REQUIRE(reg.size() == 3);
    reg.destroy(a);
    reg.destroy(b);
    REQUIRE(reg.size() == 1);
}

TEST_CASE("IEntityRegistry -clear removes all entities", "[domain][shared][ecs]") {
    StubEntityRegistry reg;
    reg.create();
    reg.create();
    reg.clear();
    REQUIRE(reg.size() == 0);
}

TEST_CASE("IEntityRegistry -isAlive returns false for unknown id", "[domain][shared][ecs]") {
    StubEntityRegistry reg;
    REQUIRE_FALSE(reg.isAlive(999));
}

}  // namespace mycad::domain::tests
