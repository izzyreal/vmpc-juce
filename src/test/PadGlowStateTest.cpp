#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "gui/vector/PadGlowState.hpp"
#include "concurrency/TaskQueue.hpp"

using namespace vmpc_juce::gui::vector;
using namespace mpc;
using mpc::controller::Bank;

namespace
{
    constexpr ProgramPadIndex a3{2};
    constexpr ProgramPadIndex b3{18};
    constexpr ProgramPadIndex c3{34};
    constexpr ProgramPadIndex d3{50};

    void advance(PadGlowState &state, const int ticks)
    {
        for (int tick = 0; tick < ticks; ++tick)
        {
            state.markPhysicalGlowDisplayed();
            state.markProgramGlowsDisplayed();
            state.advance();
        }
    }
} // namespace

TEST_CASE("A queued program release survives a model bank change", "[pad-glow]")
{
    PadGlowState state;
    state.setActiveBank(Bank::B);
    state.registerProgramPress(a3, MaxVelocity);
    advance(state, 20);
    REQUIRE(state.getLayers().otherBanks == Catch::Approx(0.6f));

    // Reproduce the original ordering with just two accepted UI tasks. The
    // controller bank changes before either task runs, but the displayed bank
    // changes only when its callback runs. The release must use A3's identity.
    concurrency::TaskQueue ui;
    utils::Task release(
        [&state]
        {
            state.registerProgramRelease(a3);
        });
    REQUIRE(ui.post(release));
    const Bank modelBank = Bank::A;
    utils::Task bankSwitch(
        [&state, modelBank]
        {
            state.setActiveBank(modelBank);
        });
    REQUIRE(ui.post(bankSwitch));
    ui.drain();

    CHECK(state.getLayers().selectedBank == Catch::Approx(0.6f));
    CHECK(state.getLayers().otherBanks == 0.f);
    advance(state, 100);
    CHECK(state.getLayers().selectedBank == 0.f);
    CHECK(state.getLayers().otherBanks == 0.f);
}

TEST_CASE("Bank changes recolor sustained and fading glows without restarting",
          "[pad-glow]")
{
    PadGlowState state;
    state.registerProgramPress(a3, MaxVelocity);
    advance(state, 20);
    SECTION("Sustained glow") {}
    SECTION("Released glow")
    {
        REQUIRE(state.registerProgramRelease(a3));
        advance(state, 7);
    }
    const float alpha = state.getLayers().selectedBank;
    REQUIRE(alpha > 0.f);
    for (const auto bank : {Bank::B, Bank::C, Bank::D, Bank::A})
    {
        state.setActiveBank(bank);
        const auto layers = state.getLayers();
        CHECK(layers.selectedBank == (bank == Bank::A ? alpha : 0.f));
        CHECK(layers.otherBanks == (bank == Bank::A ? 0.f : alpha));
    }
    state.registerProgramRelease(a3);
    advance(state, 100);
    CHECK(state.getLayers().selectedBank == 0.f);
    CHECK(state.getLayers().otherBanks == 0.f);
}

TEST_CASE("Overlapping banks keep independent glow lifetimes and layers",
          "[pad-glow]")
{
    PadGlowState state;
    const std::array<ProgramPadIndex, 4> indices{a3, b3, c3, d3};
    const std::array<int, 4> velocities{127, 110, 90, 70};
    for (size_t bank = 0; bank < indices.size(); ++bank)
    {
        state.registerProgramPress(indices[bank], Velocity(velocities[bank]));
    }
    advance(state, 20);
    for (size_t bank = 0; bank < indices.size(); ++bank)
    {
        state.setActiveBank(static_cast<Bank>(bank));
        const auto layers = state.getLayers();
        CHECK(layers.selectedBank ==
              Catch::Approx(0.6f * velocities[bank] / 127.f));
        const float strongestOther = bank == 0 ? 110.f : 127.f;
        CHECK(layers.otherBanks ==
              Catch::Approx(0.6f * strongestOther / 127.f));
    }

    // Release the strongest blue glow while other off-bank notes remain held.
    state.setActiveBank(Bank::D);
    state.registerProgramRelease(a3);
    advance(state, 100);
    CHECK(state.getLayers().otherBanks == Catch::Approx(0.6f * 110.f / 127.f));
    CHECK(state.getLayers().selectedBank == Catch::Approx(0.6f * 70.f / 127.f));

    for (const auto index : {c3, d3, b3})
    {
        state.setActiveBank(controller::programPadIndexToBank(index));
        REQUIRE(state.registerProgramRelease(index));
        advance(state, 3);
    }
    advance(state, 100);
    CHECK(state.getLayers().selectedBank == 0.f);
    CHECK(state.getLayers().otherBanks == 0.f);
}

TEST_CASE("Weaker off-bank glows fade even when the blue layer hides them",
          "[pad-glow]")
{
    PadGlowState state;
    state.registerProgramPress(b3, MaxVelocity);
    state.registerProgramPress(c3, Velocity(64));
    state.registerProgramRelease(c3);
    advance(state, 100);
    CHECK(state.getLayers().otherBanks == Catch::Approx(0.6f));
    state.setActiveBank(Bank::C);
    CHECK(state.getLayers().selectedBank == 0.f);
    state.registerProgramRelease(b3);
    advance(state, 100);
    CHECK(state.getLayers().otherBanks == 0.f);
}

TEST_CASE("Program retriggers preserve counts across bank changes and fades",
          "[pad-glow]")
{
    PadGlowState state;
    state.registerProgramPress(a3, MaxVelocity);
    state.registerProgramPress(a3, MaxVelocity);
    state.registerProgramRelease(a3);
    state.setActiveBank(Bank::B);
    advance(state, 100);
    CHECK(state.getLayers().otherBanks == Catch::Approx(0.6f));
    REQUIRE(state.registerProgramRelease(a3));
    advance(state, 8);
    CHECK(state.getLayers().otherBanks < 0.6f);

    state.registerProgramPress(a3, MaxVelocity);
    state.setActiveBank(Bank::A);
    CHECK(state.getLayers().selectedBank == 1.f);
    advance(state, 100);
    CHECK(state.getLayers().selectedBank == Catch::Approx(0.6f));
    REQUIRE(state.registerProgramRelease(a3));
    CHECK_FALSE(state.registerProgramRelease(a3));
    advance(state, 100);
    CHECK(state.getLayers().selectedBank == 0.f);
}

TEST_CASE("Physical feedback and program aftertouch remain independent",
          "[pad-glow]")
{
    PadGlowState state;
    CHECK_FALSE(state.registerPhysicalAftertouch(Pressure(64)));
    CHECK_FALSE(state.registerPhysicalRelease());
    CHECK_FALSE(state.registerProgramAftertouch(a3, Pressure(64)));
    CHECK_FALSE(state.registerProgramRelease(a3));

    state.registerPhysicalPress(MaxVelocity);
    state.registerProgramPress(a3, MaxVelocity);
    state.registerProgramPress(b3, MaxVelocity);
    REQUIRE(state.registerProgramAftertouch(b3, Pressure(64)));
    advance(state, 100);
    CHECK(state.isPhysicallyPressed());
    CHECK(state.getLayers().physical == Catch::Approx(0.8f));
    CHECK(state.getLayers().selectedBank == Catch::Approx(0.6f));
    CHECK(state.getLayers().otherBanks == Catch::Approx(0.6f * 64.f / 127.f));

    state.setActiveBank(Bank::B);
    state.registerProgramRelease(a3);
    state.registerProgramRelease(b3);
    advance(state, 100);
    CHECK(state.isPhysicallyPressed());
    CHECK(state.getLayers().physical == Catch::Approx(0.8f));
    CHECK(state.getLayers().selectedBank == 0.f);
    CHECK(state.getLayers().otherBanks == 0.f);

    REQUIRE(state.registerPhysicalAftertouch(Pressure(64)));
    CHECK(state.getLayers().physical == Catch::Approx(0.8f * 64.f / 127.f));
    state.registerPhysicalRelease();
    CHECK_FALSE(state.isPhysicallyPressed());
    advance(state, 100);
    CHECK(state.getLayers().physical == 0.f);
}
