#include <catch2/catch_test_macros.hpp>

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/vector/DisclaimerLayout.hpp"

using namespace vmpc_juce::gui::vector;

namespace
{
    void checkLayout(const float width, const float height,
                     const float baseFontHeight, const float scaleMultiplier)
    {
        juce::Font font;
        font.setHeight(baseFontHeight);
        const juce::Rectangle<float> componentBounds{width, height};
        const auto layout = disclaimer_layout::calculate(componentBounds, font,
                                                         scaleMultiplier);

        CHECK(componentBounds.contains(layout.panelBounds));
        CHECK(layout.panelBounds.contains(layout.contentBounds));
        CHECK(layout.text.getWidth() <=
              layout.contentBounds.getWidth() + 0.01f);
        CHECK(layout.text.getHeight() <=
              layout.contentBounds.getHeight() + 0.01f);
        CHECK(layout.fontHeight > 0.f);
    }
} // namespace

TEST_CASE("Disclaimer text remains inside its responsive panel",
          "[vmpc][disclaimer][layout]")
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    SECTION("phone portrait")
    {
        checkLayout(390.f, 844.f, 6.2f, 2.f);
    }

    SECTION("phone landscape")
    {
        checkLayout(675.f, 312.f, 6.2f, 1.6f);
    }

    SECTION("compact tablet or desktop overlay")
    {
        checkLayout(222.f, 171.f, 6.2f, 1.f);
    }

    SECTION("unusually narrow overlay")
    {
        checkLayout(120.f, 160.f, 6.2f, 2.f);
    }
}
