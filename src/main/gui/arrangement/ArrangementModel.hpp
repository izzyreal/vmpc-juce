#pragma once

#include <cstdint>
#include <array>
#include <optional>
#include <string>
#include <vector>

namespace vmpc_juce::gui::arrangement
{
    inline constexpr auto arrangementFileExtension = "vmpc_gui_arrangement";
    inline constexpr auto arrangementSetupFileExtension =
        "vmpc_gui_arrangements";

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

    struct LogicalRect
    {
        LogicalPoint position;
        LogicalSize size;
    };

    enum class ResizeCorner
    {
        topLeft,
        topRight,
        bottomLeft,
        bottomRight
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

    struct ArrangementNodeModel
    {
        std::uint64_t id = 0;
        // Preferred centre in the normalized 0..1 document canvas.
        LogicalPoint center{0.5f, 0.5f};
        // Width as a fraction of the target canvas width. Height follows the
        // component's reference aspect ratio.
        float widthFraction = 0.25f;
        LogicalSize referenceSize;
        std::string catalogId;
    };

    struct ArrangementDocument
    {
        std::vector<ArrangementNodeModel> nodes;
    };

    struct ArrangementSlot
    {
        std::string id;
        Orientation orientation = Orientation::portrait;
        ArrangementDocument arrangement;
    };

    struct ArrangementSetup
    {
        static constexpr std::size_t slotCount = 5;
        std::array<std::optional<ArrangementSlot>, slotCount> slots;
    };

    struct ProjectedNodeGeometry
    {
        LogicalPoint position;
        LogicalSize size;
        float scale = 1.f;
    };

    struct ProjectedArrangementNode
    {
        std::uint64_t id = 0;
        ProjectedNodeGeometry geometry;
    };

    struct ResponsiveLayout
    {
        float sharedScale = 1.f;
        bool hasValidPlacement = true;
        std::vector<ProjectedArrangementNode> nodes;
    };

    const std::vector<DeviceProfile> &getDeviceProfiles();
    LogicalSize getEffectiveDeviceSize(const DeviceProfile &device,
                                       Orientation orientation);
    float snapToGrid(float value, float gridSize = 4.f);
    float snapItemScaleToGrid(float requestedScale, LogicalSize referenceSize,
                              float gridSize = 4.f);
    float constrainItemScale(float requestedScale, LogicalSize referenceSize,
                             LogicalSize deviceSize, float minimumScale = 0.5f,
                             float maximumScale = 6.f);
    LogicalPoint constrainItemPosition(LogicalPoint requestedPosition,
                                       LogicalSize itemSize,
                                       LogicalSize deviceSize,
                                       bool shouldSnapToGrid,
                                       float gridSize = 4.f);
    ProjectedNodeGeometry projectNode(const ArrangementNodeModel &node,
                                      LogicalSize targetSize);
    ProjectedNodeGeometry projectNodeAtScale(const ArrangementNodeModel &node,
                                             LogicalSize targetSize,
                                             float sharedScale);
    LogicalPoint normalizedCenter(ProjectedNodeGeometry geometry,
                                  LogicalSize targetSize);
    ResponsiveLayout projectDocumentAtScale(const ArrangementDocument &document,
                                            LogicalSize targetSize,
                                            float sharedScale);
    ResponsiveLayout
    computeResponsiveLayout(const ArrangementDocument &document,
                            LogicalSize targetSize);
    const ProjectedNodeGeometry *
    findProjectedGeometry(const ResponsiveLayout &layout, std::uint64_t nodeId);
    bool rectanglesOverlap(LogicalRect first, LogicalRect second);
    bool isPlacementValid(LogicalRect candidate, LogicalSize bounds,
                          const std::vector<LogicalRect> &obstacles);
    std::optional<LogicalPoint> findNearestAvailablePosition(
        LogicalPoint requestedPosition, LogicalSize itemSize,
        LogicalSize bounds, const std::vector<LogicalRect> &obstacles,
        bool shouldSnapToGrid = false, float gridSize = 4.f);
    std::optional<LogicalPoint> findNearestAvailableAxisTranslation(
        LogicalPoint requestedDelta,
        const std::vector<LogicalRect> &movingItems, LogicalSize bounds,
        const std::vector<LogicalRect> &obstacles, float searchStep = 0.f);
    std::string
    serializeArrangementDocument(const ArrangementDocument &document);
    std::optional<ArrangementDocument>
    deserializeArrangementDocument(const std::string &contents,
                                   std::string &errorMessage);
    std::string serializeArrangementSetup(const ArrangementSetup &setup);
    std::optional<ArrangementSetup>
    deserializeArrangementSetup(const std::string &contents,
                                std::string &errorMessage);
    std::string createArrangementId();
    bool isValidArrangementId(const std::string &id);
    std::optional<std::size_t>
    findArrangementSlotById(const ArrangementSetup &setup,
                            const std::string &id);
    std::optional<std::size_t>
    resolveArrangementSlot(const ArrangementSetup &setup,
                           const std::optional<std::string> &preferredId);
    std::optional<std::size_t>
    findFirstOccupiedSlot(const ArrangementSetup &setup);
    LogicalPoint positionForResizedNode(ProjectedNodeGeometry startGeometry,
                                        LogicalSize requestedSize,
                                        ResizeCorner corner,
                                        bool useCentrePivot);
} // namespace vmpc_juce::gui::arrangement
