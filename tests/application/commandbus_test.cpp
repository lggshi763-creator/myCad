#include <catch2/catch_test_macros.hpp>
#include <mycad/application/CommandBus.hpp>
#include <mycad/domain/IEventStore.hpp>

// ---------------------------------------------------------------------------
// Minimal stub event store (no-op implementation for CommandBus tests)
// ---------------------------------------------------------------------------

namespace {

class StubEventStore final : public mycad::domain::IEventStore {
public:
    void append(mycad::domain::AggregateId,
                mycad::domain::Version,
                std::span<const mycad::domain::DomainEvent* const>) override {}

    [[nodiscard]] std::vector<std::unique_ptr<mycad::domain::DomainEvent>>
    load(mycad::domain::AggregateId) override {
        return {};
    }

    [[nodiscard]] std::vector<std::unique_ptr<mycad::domain::DomainEvent>>
    loadSince(mycad::domain::AggregateId, mycad::domain::Version) override {
        return {};
    }

    [[nodiscard]] mycad::domain::Version latestVersion(mycad::domain::AggregateId) override {
        return mycad::domain::Version{};
    }
};

// ---------------------------------------------------------------------------
// Test command types
// ---------------------------------------------------------------------------

struct PingCommand {
    int value{0};
};

struct OtherCommand {
    std::string tag;
};

// ---------------------------------------------------------------------------
// Test handlers
// ---------------------------------------------------------------------------

class PingHandler final : public mycad::application::ICommandHandler<PingCommand> {
public:
    int lastValue{-1};
    std::string lastSource;

    mycad::application::CommandResult
    handle(const PingCommand& cmd, const mycad::application::CommandContext& ctx) override {
        lastValue = cmd.value;
        lastSource = ctx.source;
        return {};
    }
};

class FailingHandler final : public mycad::application::ICommandHandler<PingCommand> {
public:
    mycad::application::CommandResult handle(const PingCommand&,
                                             const mycad::application::CommandContext&) override {
        return std::unexpected(mycad::application::CommandError{
            mycad::application::CommandErrorKind::BusinessRuleViolated, "always fails"});
    }
};

}  // namespace

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static mycad::application::CommandBus makebus() {
    return mycad::application::CommandBus{std::make_shared<StubEventStore>()};
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST_CASE("CommandBus - send with no handler returns HandlerNotFound", "[app][commandbus]") {
    auto bus = makebus();
    auto res = bus.send(PingCommand{42});
    REQUIRE(!res);
    CHECK(res.error().kind == mycad::application::CommandErrorKind::HandlerNotFound);
}

TEST_CASE("CommandBus - register and send dispatches to handler", "[app][commandbus]") {
    auto bus = makebus();
    auto handler = std::make_shared<PingHandler>();
    bus.registerHandler<PingCommand>(handler);

    auto res = bus.send(PingCommand{7});
    CHECK(res.has_value());
    CHECK(handler->lastValue == 7);
}

TEST_CASE("CommandBus - handler receives correct command value", "[app][commandbus]") {
    auto bus = makebus();
    auto handler = std::make_shared<PingHandler>();
    bus.registerHandler<PingCommand>(handler);

    (void)bus.send(PingCommand{99});
    CHECK(handler->lastValue == 99);
}

TEST_CASE("CommandBus - handler receives CommandContext", "[app][commandbus]") {
    auto bus = makebus();
    auto handler = std::make_shared<PingHandler>();
    bus.registerHandler<PingCommand>(handler);

    mycad::application::CommandContext ctx;
    ctx.source = "test-suite";
    (void)bus.send(PingCommand{1}, ctx);
    CHECK(handler->lastSource == "test-suite");
}

TEST_CASE("CommandBus - two command types register independently", "[app][commandbus]") {
    auto bus = makebus();
    auto ping = std::make_shared<PingHandler>();

    bool otherCalled = false;
    class OtherHandler final : public mycad::application::ICommandHandler<OtherCommand> {
    public:
        bool* called;
        explicit OtherHandler(bool* c) : called{c} {}
        mycad::application::CommandResult
        handle(const OtherCommand&, const mycad::application::CommandContext&) override {
            *called = true;
            return {};
        }
    };
    auto other = std::make_shared<OtherHandler>(&otherCalled);

    bus.registerHandler<PingCommand>(ping);
    bus.registerHandler<OtherCommand>(other);

    (void)bus.send(PingCommand{5});
    (void)bus.send(OtherCommand{"hi"});

    CHECK(ping->lastValue == 5);
    CHECK(otherCalled);
}

TEST_CASE("CommandBus - handler error propagates to caller", "[app][commandbus]") {
    auto bus = makebus();
    auto handler = std::make_shared<FailingHandler>();
    bus.registerHandler<PingCommand>(handler);

    auto res = bus.send(PingCommand{0});
    REQUIRE(!res);
    CHECK(res.error().kind == mycad::application::CommandErrorKind::BusinessRuleViolated);
    CHECK(res.error().message == "always fails");
}

TEST_CASE("CommandBus - re-registering replaces previous handler", "[app][commandbus]") {
    auto bus = makebus();
    auto h1 = std::make_shared<PingHandler>();
    auto h2 = std::make_shared<PingHandler>();
    bus.registerHandler<PingCommand>(h1);
    bus.registerHandler<PingCommand>(h2);

    (void)bus.send(PingCommand{42});
    CHECK(h1->lastValue == -1);  // h1 never called
    CHECK(h2->lastValue == 42);  // h2 is active
}

TEST_CASE("CommandBus - eventStore returns injected store", "[app][commandbus]") {
    auto store = std::make_shared<StubEventStore>();
    mycad::application::CommandBus bus{store};
    CHECK(bus.eventStore().get() == store.get());
}
