#include <catch2/catch_test_macros.hpp>

#include "gui/vector/AuxLcdMenuModel.hpp"

using namespace vmpc_juce::gui::vector;

TEST_CASE("Auxiliary LCD menu state follows its toggle action",
          "[vmpc][menu][aux-lcd]")
{
    auto auxLcdOpen = false;
    auto toggleCount = 0;
    const std::function<bool()> toggleAuxLcd = [&]
    {
        ++toggleCount;
        auxLcdOpen = !auxLcdOpen;
        return auxLcdOpen;
    };
    AuxLcdMenuModel model(toggleAuxLcd);

    CHECK_FALSE(model.isOpen());
    CHECK(model.tooltipText() == "Open auxiliary LCD");
    model.activate();
    CHECK(model.isOpen());
    CHECK(model.tooltipText() == "Close auxiliary LCD");
    CHECK(toggleCount == 1);
    model.activate();
    CHECK_FALSE(model.isOpen());
    CHECK(model.tooltipText() == "Open auxiliary LCD");
    CHECK(toggleCount == 2);

    CHECK(model.setOpen(true));
    CHECK_FALSE(model.setOpen(true));
}
