#include "gui/arrangement/ArrangementModel.hpp"
#include "gui/arrangement/ArrangementCatalog.hpp"

#include <juce_core/juce_core.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <unordered_set>

using namespace vmpc_juce::gui::arrangement;

namespace
{
    using json = nlohmann::json;

    constexpr auto designFormat = "vmpc2000xl-arrangement";
    constexpr int designFormatVersion = 4;
    constexpr auto setupFormat = "vmpc2000xl-arrangement-setup";
    constexpr int setupFormatVersion = 4;

    const char *orientationName(const Orientation orientation)
    {
        return orientation == Orientation::landscape ? "landscape" : "portrait";
    }

    Orientation parseOrientation(const json &value)
    {
        const auto name = value.get<std::string>();
        if (name == "portrait")
        {
            return Orientation::portrait;
        }
        if (name == "landscape")
        {
            return Orientation::landscape;
        }
        throw std::runtime_error("orientation must be portrait or landscape");
    }

    json pointToJson(const LogicalPoint point)
    {
        return {{"x", point.x}, {"y", point.y}};
    }

    json sizeToJson(const LogicalSize size)
    {
        return {{"width", size.width}, {"height", size.height}};
    }

    LogicalPoint parsePoint(const json &value)
    {
        return {value.at("x").get<float>(), value.at("y").get<float>()};
    }

    LogicalSize parseSize(const json &value)
    {
        return {value.at("width").get<float>(),
                value.at("height").get<float>()};
    }

    json nodeToJson(const ArrangementNodeModel &node)
    {
        json result{{"id", node.id},
                    {"component", node.catalogId},
                    {"center", pointToJson(node.center)},
                    {"widthFraction", node.widthFraction},
                    {"referenceSize", sizeToJson(node.referenceSize)}};
        return result;
    }

    ArrangementNodeModel parseNode(const json &value)
    {
        ArrangementNodeModel result;
        result.id = value.at("id").get<std::uint64_t>();
        result.center = parsePoint(value.at("center"));
        result.widthFraction = value.at("widthFraction").get<float>();
        result.referenceSize = parseSize(value.at("referenceSize"));
        result.catalogId = value.at("component").get<std::string>();
        return result;
    }

    bool validPoint(const LogicalPoint point)
    {
        return std::isfinite(point.x) && std::isfinite(point.y);
    }

    bool validSize(const LogicalSize size)
    {
        return std::isfinite(size.width) && std::isfinite(size.height) &&
               size.width > 0.f && size.height > 0.f;
    }

    bool validateDocument(const ArrangementDocument &document,
                          std::string &error)
    {
        std::unordered_set<std::uint64_t> ids;
        for (const auto &node : document.nodes)
        {
            if (node.id == 0 ||
                node.id == std::numeric_limits<std::uint64_t>::max() ||
                !ids.insert(node.id).second || node.catalogId.empty())
            {
                error = "The design contains an invalid component ID or type.";
                return false;
            }
            if (!validPoint(node.center) || node.center.x < 0.f ||
                node.center.x > 1.f || node.center.y < 0.f ||
                node.center.y > 1.f || !std::isfinite(node.widthFraction) ||
                node.widthFraction <= 0.f || node.widthFraction > 1.f ||
                !validSize(node.referenceSize))
            {
                error = "The design contains invalid normalized geometry.";
                return false;
            }
        }
        return true;
    }
} // namespace

const std::vector<DeviceProfile> &
vmpc_juce::gui::arrangement::getDeviceProfiles()
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

LogicalSize vmpc_juce::gui::arrangement::getEffectiveDeviceSize(
    const DeviceProfile &device, const Orientation orientation)
{
    if (orientation == Orientation::landscape)
    {
        return {static_cast<float>(device.portraitHeight),
                static_cast<float>(device.portraitWidth)};
    }

    return {static_cast<float>(device.portraitWidth),
            static_cast<float>(device.portraitHeight)};
}

float vmpc_juce::gui::arrangement::snapToGrid(const float value,
                                              const float gridSize)
{
    if (gridSize <= 0.f)
    {
        return value;
    }

    return std::round(value / gridSize) * gridSize;
}

float vmpc_juce::gui::arrangement::snapAxisTranslationToGrid(
    const float position, const float requestedTranslation,
    const float gridSize)
{
    if (gridSize <= 0.f || requestedTranslation == 0.f)
    {
        return requestedTranslation;
    }

    constexpr float alignmentTolerance = 0.001f;
    const auto gridIndex = position / gridSize;
    const auto nearestGridIndex = std::round(gridIndex);
    const auto isAligned =
        std::abs(position - nearestGridIndex * gridSize) <= alignmentTolerance;
    const auto direction = requestedTranslation > 0.f ? 1.f : -1.f;
    const auto gridSteps =
        std::max(1.f, std::round(std::abs(requestedTranslation) / gridSize));
    const auto startingGridIndex = isAligned         ? nearestGridIndex
                                   : direction > 0.f ? std::floor(gridIndex)
                                                     : std::ceil(gridIndex);
    const auto targetGridIndex = startingGridIndex + direction * gridSteps;
    return targetGridIndex * gridSize - position;
}

float vmpc_juce::gui::arrangement::snapItemScaleToGrid(
    const float requestedScale, const LogicalSize referenceSize,
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

float vmpc_juce::gui::arrangement::constrainItemScale(
    const float requestedScale, const LogicalSize referenceSize,
    const LogicalSize deviceSize, const float minimumScale,
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

LogicalPoint vmpc_juce::gui::arrangement::constrainItemPosition(
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

namespace
{
    bool responsiveLayoutIsValid(const ResponsiveLayout &layout,
                                 const LogicalSize targetSize)
    {
        constexpr float epsilon = 0.001f;
        for (const auto &node : layout.nodes)
        {
            const auto &geometry = node.geometry;
            if (geometry.position.x < -epsilon ||
                geometry.position.y < -epsilon ||
                geometry.position.x + geometry.size.width >
                    targetSize.width + epsilon ||
                geometry.position.y + geometry.size.height >
                    targetSize.height + epsilon)
            {
                return false;
            }
        }

        for (size_t i = 0; i < layout.nodes.size(); ++i)
        {
            const auto &first = layout.nodes[i].geometry;
            for (size_t j = i + 1; j < layout.nodes.size(); ++j)
            {
                const auto &second = layout.nodes[j].geometry;
                if (rectanglesOverlap({first.position, first.size},
                                      {second.position, second.size}))
                {
                    return false;
                }
            }
        }
        return true;
    }

} // namespace

ProjectedNodeGeometry
vmpc_juce::gui::arrangement::projectNode(const ArrangementNodeModel &node,
                                         const LogicalSize targetSize)
{
    return projectNodeAtScale(node, targetSize, 1.f);
}

ProjectedNodeGeometry vmpc_juce::gui::arrangement::projectNodeAtScale(
    const ArrangementNodeModel &node, const LogicalSize targetSize,
    const float uniformScale)
{
    if (node.referenceSize.width <= 0.f || targetSize.width <= 0.f ||
        targetSize.height <= 0.f)
    {
        return {};
    }
    const auto fit = std::max(0.f, uniformScale);
    const auto authoredWidth = node.widthFraction * targetSize.width;
    const auto authoredScale = authoredWidth / node.referenceSize.width;
    const auto width = authoredWidth * fit;
    const auto projectedScale = authoredScale * fit;
    const LogicalSize size{width,
                           node.referenceSize.height * projectedScale};
    const LogicalPoint center{node.center.x * targetSize.width,
                              node.center.y * targetSize.height};
    return {{center.x - size.width * 0.5f, center.y - size.height * 0.5f},
            size,
            projectedScale};
}

ResponsiveLayout vmpc_juce::gui::arrangement::projectDocumentAtScale(
    const ArrangementDocument &document, const LogicalSize targetSize,
    const float uniformScale)
{
    ResponsiveLayout result;
    result.uniformScale = std::max(0.f, uniformScale);
    result.nodes.reserve(document.nodes.size());
    for (const auto &node : document.nodes)
    {
        result.nodes.push_back(
            {node.id, projectNodeAtScale(node, targetSize,
                                         result.uniformScale)});
    }
    result.hasValidPlacement = responsiveLayoutIsValid(result, targetSize);
    return result;
}

LogicalPoint vmpc_juce::gui::arrangement::normalizedCenter(
    const ProjectedNodeGeometry geometry, const LogicalSize targetSize)
{
    if (targetSize.width <= 0.f || targetSize.height <= 0.f)
    {
        return {};
    }
    return {std::clamp((geometry.position.x + geometry.size.width * 0.5f) /
                           targetSize.width,
                       0.f, 1.f),
            std::clamp((geometry.position.y + geometry.size.height * 0.5f) /
                           targetSize.height,
                       0.f, 1.f)};
}

ResponsiveLayout vmpc_juce::gui::arrangement::computeResponsiveLayout(
    const ArrangementDocument &document, const LogicalSize targetSize)
{
    auto fullSize = projectDocumentAtScale(document, targetSize, 1.f);
    if (fullSize.hasValidPlacement || document.nodes.empty())
    {
        return fullSize;
    }

    auto lower = 0.f;
    auto upper = 1.f;
    auto result = projectDocumentAtScale(document, targetSize, lower);
    if (!result.hasValidPlacement)
    {
        return result;
    }

    for (int iteration = 0; iteration < 40; ++iteration)
    {
        const auto candidate = (lower + upper) * 0.5f;
        auto candidateLayout =
            projectDocumentAtScale(document, targetSize, candidate);
        if (candidateLayout.hasValidPlacement)
        {
            lower = candidate;
            result = std::move(candidateLayout);
        }
        else
        {
            upper = candidate;
        }
    }
    return result;
}

PixelBounds vmpc_juce::gui::arrangement::roundedPixelBounds(
    const ProjectedNodeGeometry geometry)
{
    const auto left = static_cast<int>(std::lround(geometry.position.x));
    const auto top = static_cast<int>(std::lround(geometry.position.y));
    const auto right = static_cast<int>(
        std::lround(geometry.position.x + geometry.size.width));
    const auto bottom = static_cast<int>(
        std::lround(geometry.position.y + geometry.size.height));
    return {left, top, std::max(0, right - left),
            std::max(0, bottom - top)};
}

const ProjectedNodeGeometry *vmpc_juce::gui::arrangement::findProjectedGeometry(
    const ResponsiveLayout &layout, const std::uint64_t nodeId)
{
    const auto found = std::find_if(layout.nodes.begin(), layout.nodes.end(),
                                    [nodeId](const auto &node)
                                    {
                                        return node.id == nodeId;
                                    });
    return found == layout.nodes.end() ? nullptr : &found->geometry;
}

bool vmpc_juce::gui::arrangement::rectanglesOverlap(const LogicalRect first,
                                                    const LogicalRect second)
{
    return first.position.x + first.size.width > second.position.x &&
           second.position.x + second.size.width > first.position.x &&
           first.position.y + first.size.height > second.position.y &&
           second.position.y + second.size.height > first.position.y;
}

bool vmpc_juce::gui::arrangement::isPlacementValid(
    const LogicalRect candidate, const LogicalSize bounds,
    const std::vector<LogicalRect> &obstacles)
{
    constexpr float epsilon = 0.001f;
    if (candidate.position.x < -epsilon || candidate.position.y < -epsilon ||
        candidate.position.x + candidate.size.width > bounds.width + epsilon ||
        candidate.position.y + candidate.size.height > bounds.height + epsilon)
    {
        return false;
    }
    return std::none_of(obstacles.begin(), obstacles.end(),
                        [candidate](const auto obstacle)
                        {
                            return rectanglesOverlap(candidate, obstacle);
                        });
}

std::optional<LogicalPoint>
vmpc_juce::gui::arrangement::findNearestAvailablePosition(
    const LogicalPoint requestedPosition, const LogicalSize itemSize,
    const LogicalSize bounds, const std::vector<LogicalRect> &obstacles,
    const bool shouldSnapToGrid, const float gridSize)
{
    const auto constrainedRequest = constrainItemPosition(
        requestedPosition, itemSize, bounds, shouldSnapToGrid, gridSize);
    std::vector<float> candidateXs{constrainedRequest.x, 0.f,
                                   bounds.width - itemSize.width};
    std::vector<float> candidateYs{constrainedRequest.y, 0.f,
                                   bounds.height - itemSize.height};
    for (const auto obstacle : obstacles)
    {
        candidateXs.push_back(obstacle.position.x - itemSize.width);
        candidateXs.push_back(obstacle.position.x + obstacle.size.width);
        candidateYs.push_back(obstacle.position.y - itemSize.height);
        candidateYs.push_back(obstacle.position.y + obstacle.size.height);
        if (shouldSnapToGrid && gridSize > 0.f)
        {
            candidateXs.push_back(
                std::floor((obstacle.position.x - itemSize.width) / gridSize) *
                gridSize);
            candidateXs.push_back(
                std::ceil((obstacle.position.x + obstacle.size.width) /
                          gridSize) *
                gridSize);
            candidateYs.push_back(
                std::floor((obstacle.position.y - itemSize.height) / gridSize) *
                gridSize);
            candidateYs.push_back(
                std::ceil((obstacle.position.y + obstacle.size.height) /
                          gridSize) *
                gridSize);
        }
    }

    auto bestDistance = std::numeric_limits<float>::max();
    std::optional<LogicalPoint> best;
    for (const auto x : candidateXs)
    {
        for (const auto y : candidateYs)
        {
            const auto position = constrainItemPosition(
                {x, y}, itemSize, bounds, shouldSnapToGrid, gridSize);
            const LogicalRect placed{position, itemSize};
            constexpr float epsilon = 0.001f;
            const auto isInBounds =
                position.x >= -epsilon && position.y >= -epsilon &&
                position.x + itemSize.width <= bounds.width + epsilon &&
                position.y + itemSize.height <= bounds.height + epsilon;
            if (!isInBounds ||
                std::any_of(obstacles.begin(), obstacles.end(),
                            [placed](const auto obstacle)
                            {
                                return rectanglesOverlap(placed, obstacle);
                            }))
            {
                continue;
            }

            const auto dx = position.x - constrainedRequest.x;
            const auto dy = position.y - constrainedRequest.y;
            const auto distance = dx * dx + dy * dy;
            if (!best.has_value() || distance < bestDistance - 0.001f ||
                (std::abs(distance - bestDistance) <= 0.001f &&
                 (position.y < best->y ||
                  (std::abs(position.y - best->y) <= 0.001f &&
                   position.x < best->x))))
            {
                best = position;
                bestDistance = distance;
            }
        }
    }
    return best;
}

std::optional<LogicalPoint>
vmpc_juce::gui::arrangement::findNearestAvailableAxisTranslation(
    const LogicalPoint requestedDelta,
    const std::vector<LogicalRect> &movingItems, const LogicalSize bounds,
    const std::vector<LogicalRect> &obstacles, const float searchStep)
{
    constexpr float epsilon = 0.001f;
    const auto horizontal = std::abs(requestedDelta.x) > epsilon &&
                            std::abs(requestedDelta.y) <= epsilon;
    const auto vertical = std::abs(requestedDelta.y) > epsilon &&
                          std::abs(requestedDelta.x) <= epsilon;
    if ((!horizontal && !vertical) || movingItems.empty())
    {
        return std::nullopt;
    }

    const auto requested =
        std::abs(horizontal ? requestedDelta.x : requestedDelta.y);
    const auto direction =
        (horizontal ? requestedDelta.x : requestedDelta.y) > 0.f ? 1.f : -1.f;
    auto maximum = std::numeric_limits<float>::max();
    for (const auto item : movingItems)
    {
        const auto available =
            horizontal ? direction > 0.f
                             ? bounds.width - item.position.x - item.size.width
                             : item.position.x
            : direction > 0.f
                ? bounds.height - item.position.y - item.size.height
                : item.position.y;
        maximum = std::min(maximum, available);
    }
    maximum = std::max(0.f, maximum);
    if (maximum <= epsilon)
    {
        return std::nullopt;
    }

    const auto translationIsValid = [&](const float distance)
    {
        const LogicalPoint delta{horizontal ? direction * distance : 0.f,
                                 vertical ? direction * distance : 0.f};
        return std::all_of(
            movingItems.begin(), movingItems.end(),
            [&](const auto item)
            {
                return isPlacementValid(
                    {{item.position.x + delta.x, item.position.y + delta.y},
                     item.size},
                    bounds, obstacles);
            });
    };

    auto distance = std::min(requested, maximum);
    while (distance <= maximum + epsilon)
    {
        if (translationIsValid(distance))
        {
            return LogicalPoint{horizontal ? direction * distance : 0.f,
                                vertical ? direction * distance : 0.f};
        }
        if (searchStep <= epsilon || distance >= maximum - epsilon)
        {
            break;
        }
        distance = std::min(maximum, distance + searchStep);
    }
    return std::nullopt;
}

std::string vmpc_juce::gui::arrangement::serializeArrangementDocument(
    const ArrangementDocument &document)
{
    json result{{"format", designFormat},
                {"version", designFormatVersion},
                {"menuAtTop", document.menuAtTop},
                {"nodes", json::array()}};
    for (const auto &node : document.nodes)
    {
        result["nodes"].push_back(nodeToJson(node));
    }
    return result.dump(2) + "\n";
}

std::optional<ArrangementDocument>
vmpc_juce::gui::arrangement::deserializeArrangementDocument(
    const std::string &contents, std::string &errorMessage)
{
    try
    {
        const auto source = json::parse(contents);
        const auto format = source.at("format").get<std::string>();
        if (format != designFormat)
        {
            throw std::runtime_error("not a VMPC2000XL arrangement");
        }
        const auto version = source.at("version").get<int>();
        if (version != designFormatVersion)
        {
            throw std::runtime_error("unsupported design version");
        }

        ArrangementDocument result;
        result.menuAtTop = source.value("menuAtTop", false);
        const auto &nodes = source.at("nodes");
        if (!nodes.is_array())
        {
            throw std::runtime_error("nodes must be an array");
        }
        for (const auto &node : nodes)
        {
            result.nodes.push_back(parseNode(node));
        }

        if (!validateDocument(result, errorMessage))
        {
            return std::nullopt;
        }
        errorMessage.clear();
        return result;
    }
    catch (const std::exception &error)
    {
        errorMessage =
            std::string("Could not read the design: ") + error.what();
    }
    catch (...)
    {
        errorMessage = "Could not read the design: unknown file error";
    }
    return std::nullopt;
}

std::string vmpc_juce::gui::arrangement::serializeArrangementSetup(
    const ArrangementSetup &setup)
{
    json slots = json::array();
    std::unordered_set<std::string> arrangementIds;
    for (const auto &slot : setup.slots)
    {
        if (!slot.has_value())
        {
            slots.push_back(nullptr);
            continue;
        }
        if (!isValidArrangementId(slot->id) ||
            !arrangementIds.insert(slot->id).second)
        {
            throw std::runtime_error(
                "Arrangement setup IDs must be valid and unique UUIDs.");
        }
        slots.push_back(
            {{"id", slot->id},
             {"orientation", orientationName(slot->orientation)},
             {"arrangement",
              json::parse(serializeArrangementDocument(slot->arrangement))}});
    }
    return json{{"format", setupFormat},
                {"version", setupFormatVersion},
                {"slots", std::move(slots)}}
               .dump(2) +
           "\n";
}

std::optional<ArrangementSetup>
vmpc_juce::gui::arrangement::deserializeArrangementSetup(
    const std::string &contents, std::string &errorMessage)
{
    try
    {
        const auto source = json::parse(contents);
        if (source.at("format").get<std::string>() != setupFormat)
        {
            throw std::runtime_error("not a VMPC2000XL arrangement setup");
        }
        const auto version = source.at("version").get<int>();
        if (version != setupFormatVersion)
        {
            throw std::runtime_error("unsupported setup version");
        }
        const auto &slots = source.at("slots");
        if (!slots.is_array() || slots.size() != ArrangementSetup::slotCount)
        {
            throw std::runtime_error("setup must contain exactly five slots");
        }

        ArrangementSetup result;
        std::unordered_set<std::string> arrangementIds;
        for (std::size_t i = 0; i < ArrangementSetup::slotCount; ++i)
        {
            const auto &sourceSlot = slots.at(i);
            if (sourceSlot.is_null())
            {
                continue;
            }
            ArrangementSlot slot;
            slot.id = sourceSlot.at("id").get<std::string>();
            if (!isValidArrangementId(slot.id) ||
                !arrangementIds.insert(slot.id).second)
            {
                throw std::runtime_error(
                    "slot " + std::to_string(i + 1) +
                    ": arrangement ID must be a valid, unique UUID");
            }
            slot.orientation = parseOrientation(sourceSlot.at("orientation"));
            std::string documentError;
            const auto document = deserializeArrangementDocument(
                sourceSlot.at("arrangement").dump(), documentError);
            if (!document.has_value())
            {
                throw std::runtime_error("slot " + std::to_string(i + 1) +
                                         ": " + documentError);
            }
            if (!documentUsesKnownCatalogEntries(*document, documentError))
            {
                throw std::runtime_error("slot " + std::to_string(i + 1) +
                                         ": " + documentError);
            }
            slot.arrangement = *document;
            result.slots[i] = std::move(slot);
        }
        errorMessage.clear();
        return result;
    }
    catch (const std::exception &error)
    {
        errorMessage = std::string("Could not read the setup: ") + error.what();
    }
    catch (...)
    {
        errorMessage = "Could not read the setup: unknown file error";
    }
    return std::nullopt;
}

std::string vmpc_juce::gui::arrangement::createArrangementId()
{
    return juce::Uuid().toString().toStdString();
}

bool vmpc_juce::gui::arrangement::isValidArrangementId(const std::string &id)
{
    if (id.size() != 32 || !std::all_of(id.begin(), id.end(),
                                        [](const unsigned char character)
                                        {
                                            return std::isxdigit(character) !=
                                                   0;
                                        }))
    {
        return false;
    }
    const juce::Uuid uuid{juce::String(id)};
    return !uuid.isNull() && uuid.toString().equalsIgnoreCase(juce::String(id));
}

std::optional<std::size_t> vmpc_juce::gui::arrangement::findArrangementSlotById(
    const ArrangementSetup &setup, const std::string &id)
{
    for (std::size_t i = 0; i < setup.slots.size(); ++i)
    {
        if (setup.slots[i].has_value() && setup.slots[i]->id == id &&
            !setup.slots[i]->arrangement.nodes.empty())
        {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<std::size_t> vmpc_juce::gui::arrangement::resolveArrangementSlot(
    const ArrangementSetup &setup,
    const std::optional<std::string> &preferredId)
{
    if (preferredId.has_value())
    {
        if (const auto preferred = findArrangementSlotById(setup, *preferredId))
        {
            return preferred;
        }
    }
    return findFirstOccupiedSlot(setup);
}

std::optional<std::size_t> vmpc_juce::gui::arrangement::findFirstOccupiedSlot(
    const ArrangementSetup &setup)
{
    for (std::size_t i = 0; i < setup.slots.size(); ++i)
    {
        if (setup.slots[i].has_value() &&
            !setup.slots[i]->arrangement.nodes.empty())
        {
            return i;
        }
    }
    return std::nullopt;
}

LogicalPoint vmpc_juce::gui::arrangement::positionForResizedNode(
    const ProjectedNodeGeometry startGeometry, const LogicalSize requestedSize,
    const ResizeCorner corner, const bool useCentrePivot)
{
    if (useCentrePivot)
    {
        return {startGeometry.position.x + startGeometry.size.width * 0.5f -
                    requestedSize.width * 0.5f,
                startGeometry.position.y + startGeometry.size.height * 0.5f -
                    requestedSize.height * 0.5f};
    }

    switch (corner)
    {
        case ResizeCorner::topLeft:
            return {startGeometry.position.x + startGeometry.size.width -
                        requestedSize.width,
                    startGeometry.position.y + startGeometry.size.height -
                        requestedSize.height};
        case ResizeCorner::topRight:
            return {startGeometry.position.x, startGeometry.position.y +
                                                  startGeometry.size.height -
                                                  requestedSize.height};
        case ResizeCorner::bottomLeft:
            return {startGeometry.position.x + startGeometry.size.width -
                        requestedSize.width,
                    startGeometry.position.y};
        case ResizeCorner::bottomRight:
        default:
            return startGeometry.position;
    }
}
