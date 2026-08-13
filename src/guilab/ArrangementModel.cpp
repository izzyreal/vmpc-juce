#include "ArrangementModel.hpp"

#include <algorithm>
#include <cmath>

using namespace vmpc_juce::guilab;

const std::vector<DeviceProfile> &vmpc_juce::guilab::getDeviceProfiles()
{
    static const std::vector<DeviceProfile> profiles{
        {"iphone-2g-3g-3gs", "Apple", "iPhone 2G/3G/3GS", 320, 480},
        {"iphone-4-4s", "Apple", "iPhone 4/4S", 320, 480},
        {"iphone-5-5c-5s", "Apple", "iPhone 5/5C/5S", 320, 568},
        {"iphone-se", "Apple", "iPhone SE", 320, 568},
        {"iphone-6-6s-7-8-se2", "Apple", "iPhone 6/6S/7/8/SE2", 375, 667},
        {"iphone-6plus-8plus", "Apple", "iPhone 6+/6S+/7+/8+", 414, 736},
        {"iphone-xr-11", "Apple", "iPhone Xr/11", 414, 896},
        {"iphone-x-xs-11-pro", "Apple", "iPhone X/Xs/11 Pro", 375, 812},
        {"iphone-xs-max-11-pro-max", "Apple", "iPhone Xs Max/11 Pro Max", 414,
         896},
        {"iphone-12-13-pro", "Apple", "iPhone 12/12 Pro/13/13 Pro", 390, 844},
        {"iphone-12-13-pro-max", "Apple", "iPhone 12 Pro Max/13 Pro Max", 428,
         926},
        {"iphone-13-mini", "Apple", "iPhone 13 Mini", 375, 812},
        {"galaxy-s", "Samsung", "Galaxy S", 320, 533},
        {"galaxy-s2", "Samsung", "Galaxy S2", 320, 533},
        {"galaxy-s3", "Samsung", "Galaxy S3", 360, 640},
        {"galaxy-s3-mini", "Samsung", "Galaxy S3 Mini", 320, 533},
        {"galaxy-s4", "Samsung", "Galaxy S4", 360, 640},
        {"galaxy-s5", "Samsung", "Galaxy S5", 360, 640},
        {"galaxy-s6-edge", "Samsung", "Galaxy S6/Edge", 360, 640},
        {"galaxy-s7", "Samsung", "Galaxy S7", 360, 640},
        {"galaxy-s7-edge", "Samsung", "Galaxy S7 Edge", 360, 640},
        {"galaxy-s8", "Samsung", "Galaxy S8", 360, 740},
        {"galaxy-s8-plus", "Samsung", "Galaxy S8 Plus", 412, 846},
        {"galaxy-s9", "Samsung", "Galaxy S9", 360, 740},
        {"galaxy-s9-plus", "Samsung", "Galaxy S9 Plus", 412, 846},
    };

    return profiles;
}

LogicalSize
vmpc_juce::guilab::getEffectiveDeviceSize(const DeviceProfile &device,
                                          const Orientation orientation)
{
    if (orientation == Orientation::landscape)
    {
        return {static_cast<float>(device.portraitHeight),
                static_cast<float>(device.portraitWidth)};
    }

    return {static_cast<float>(device.portraitWidth),
            static_cast<float>(device.portraitHeight)};
}

float vmpc_juce::guilab::snapToGrid(const float value, const float gridSize)
{
    if (gridSize <= 0.f)
    {
        return value;
    }

    return std::round(value / gridSize) * gridSize;
}

float vmpc_juce::guilab::snapItemScaleToGrid(const float requestedScale,
                                             const LogicalSize referenceSize,
                                             const float gridSize)
{
    const auto referenceExtent =
        std::max(referenceSize.width, referenceSize.height);
    if (referenceExtent <= 0.f)
    {
        return requestedScale;
    }

    return snapToGrid(referenceExtent * requestedScale, gridSize) /
           referenceExtent;
}

float vmpc_juce::guilab::constrainItemScale(const float requestedScale,
                                            const LogicalSize referenceSize,
                                            const LogicalSize deviceSize,
                                            const float minimumScale,
                                            const float maximumScale)
{
    if (referenceSize.width <= 0.f || referenceSize.height <= 0.f)
    {
        return minimumScale;
    }

    const auto fitScale = std::min(deviceSize.width / referenceSize.width,
                                   deviceSize.height / referenceSize.height);
    const auto upper = std::max(0.f, std::min(maximumScale, fitScale));
    const auto lower = std::min(minimumScale, upper);
    return std::clamp(requestedScale, lower, upper);
}

LogicalPoint vmpc_juce::guilab::constrainItemPosition(
    LogicalPoint requestedPosition, const LogicalSize itemSize,
    const LogicalSize deviceSize, const bool shouldSnapToGrid,
    const float gridSize)
{
    if (shouldSnapToGrid)
    {
        requestedPosition.x = snapToGrid(requestedPosition.x, gridSize);
        requestedPosition.y = snapToGrid(requestedPosition.y, gridSize);
    }

    const auto maximumX = std::max(0.f, deviceSize.width - itemSize.width);
    const auto maximumY = std::max(0.f, deviceSize.height - itemSize.height);
    requestedPosition.x = std::clamp(requestedPosition.x, 0.f, maximumX);
    requestedPosition.y = std::clamp(requestedPosition.y, 0.f, maximumY);
    return requestedPosition;
}
