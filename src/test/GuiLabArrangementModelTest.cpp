#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "ArrangementModel.hpp"

#include <algorithm>
#include <string>

using namespace vmpc_juce::guilab;

TEST_CASE("GUI Lab compact LCD sizes follow the production grid",
          "[gui-lab][arrangement]")
{
    CHECK(compactDisplayReferenceSize.width ==
          Catch::Approx(229.52338f));
    CHECK(compactDisplayReferenceSize.height ==
          Catch::Approx(115.75869f));
    CHECK(compactMountedLcdReferenceSize.width ==
          compactDisplayReferenceSize.width);
    CHECK(compactMountedLcdReferenceSize.height ==
          Catch::Approx(88.19710f));
    CHECK(compactMountedLcdReferenceSize.height ==
          Catch::Approx(compactDisplayReferenceSize.height * 80.f / 105.f));
}

TEST_CASE("GUI Lab device catalog contains Apple and Samsung profiles",
          "[gui-lab][arrangement]")
{
    const auto &profiles = getDeviceProfiles();
    CHECK(profiles.size() == 25);
    CHECK(std::any_of(profiles.begin(), profiles.end(),
                      [](const auto &device)
                      {
                          return std::string(device.brand) == "Apple";
                      }));
    CHECK(std::any_of(profiles.begin(), profiles.end(),
                      [](const auto &device)
                      {
                          return std::string(device.brand) == "Samsung";
                      }));

    const auto &iphone = profiles.at(9);
    CHECK(getEffectiveDeviceSize(iphone, Orientation::portrait).width == 390.f);
    CHECK(getEffectiveDeviceSize(iphone, Orientation::portrait).height ==
          844.f);
    CHECK(getEffectiveDeviceSize(iphone, Orientation::landscape).width ==
          844.f);
    CHECK(getEffectiveDeviceSize(iphone, Orientation::landscape).height ==
          390.f);
}

TEST_CASE("GUI Lab arrangement geometry snaps and clamps items",
          "[gui-lab][arrangement]")
{
    const LogicalSize device{390.f, 844.f};
    const LogicalSize item{80.f, 48.f};

    const auto snapped =
        constrainItemPosition({13.f, 19.f}, item, device, true);
    CHECK(snapped.x == 12.f);
    CHECK(snapped.y == 20.f);

    const auto unsnapped =
        constrainItemPosition({13.f, 19.f}, item, device, false);
    CHECK(unsnapped.x == 13.f);
    CHECK(unsnapped.y == 19.f);

    const auto clamped =
        constrainItemPosition({500.f, 900.f}, item, device, true);
    CHECK(clamped.x == 310.f);
    CHECK(clamped.y == 796.f);
}

TEST_CASE("GUI Lab item scale remains proportional and fits the device",
          "[gui-lab][arrangement]")
{
    const LogicalSize reference{230.f, 115.f};
    CHECK(snapItemScaleToGrid(1.03f, {100.f, 40.f}) == 104.f / 100.f);
    CHECK(snapItemScaleToGrid(1.03f, {40.f, 100.f}) == 104.f / 100.f);
    CHECK(constrainItemScale(0.1f, reference, {390.f, 844.f}) == 0.5f);
    CHECK(constrainItemScale(3.f, reference, {390.f, 844.f}) == 390.f / 230.f);
    CHECK(constrainItemScale(1.4f, reference, {844.f, 390.f}) == 1.4f);
    CHECK(constrainItemScale(7.f, {100.f, 100.f}, {1000.f, 1000.f}) == 6.f);
}
