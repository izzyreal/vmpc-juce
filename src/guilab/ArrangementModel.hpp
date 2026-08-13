#pragma once

#include <string>
#include <vector>

namespace vmpc_juce::guilab
{
    enum class Orientation
    {
        portrait,
        landscape
    };

    struct LogicalPoint
    {
        float x = 0.f;
        float y = 0.f;
    };

    struct LogicalSize
    {
        float width = 0.f;
        float height = 0.f;
    };

    // These are the allocations produced for the compact display by
    // default_compact -> main_container -> left_side_container at scale 1.
    // Keeping the GUI Lab previews at these dimensions lets the production
    // grid fractions and scale-dependent margins reproduce the same geometry.
    inline constexpr LogicalSize compactDisplayReferenceSize{
        445.f * (215.f / 220.f) * (95.f / 180.f),
        342.f * (294.f / 303.f) * (15.f / 43.f)};
    inline constexpr LogicalSize compactMountedLcdReferenceSize{
        compactDisplayReferenceSize.width,
        compactDisplayReferenceSize.height * (80.f / 105.f)};

    struct DeviceProfile
    {
        const char *id;
        const char *brand;
        const char *name;
        int portraitWidth;
        int portraitHeight;
    };

    struct ArrangementItemModel
    {
        std::string catalogId;
        LogicalPoint position;
        float scale = 1.f;
    };

    const std::vector<DeviceProfile> &getDeviceProfiles();
    LogicalSize getEffectiveDeviceSize(const DeviceProfile &device,
                                       Orientation orientation);
    float snapToGrid(float value, float gridSize = 4.f);
    float snapItemScaleToGrid(float requestedScale, LogicalSize referenceSize,
                              float gridSize = 4.f);
    float constrainItemScale(float requestedScale, LogicalSize referenceSize,
                             LogicalSize deviceSize, float minimumScale = 0.5f,
                             float maximumScale = 3.f);
    LogicalPoint constrainItemPosition(LogicalPoint requestedPosition,
                                       LogicalSize itemSize,
                                       LogicalSize deviceSize,
                                       bool shouldSnapToGrid,
                                       float gridSize = 4.f);
} // namespace vmpc_juce::guilab
