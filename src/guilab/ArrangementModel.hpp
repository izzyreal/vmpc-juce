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
                             float maximumScale = 2.f);
    LogicalPoint constrainItemPosition(LogicalPoint requestedPosition,
                                       LogicalSize itemSize,
                                       LogicalSize deviceSize,
                                       bool shouldSnapToGrid,
                                       float gridSize = 4.f);
} // namespace vmpc_juce::guilab
