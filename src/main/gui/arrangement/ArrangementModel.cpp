#include "gui/arrangement/ArrangementModel.hpp"
#include "gui/arrangement/ArrangementCatalog.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

using namespace vmpc_juce::gui::arrangement;

namespace
{
    using json = nlohmann::json;

    constexpr auto designFormat = "vmpc2000xl-arrangement";
    constexpr auto legacyDesignFormat = "vmpc2000xl-gui-lab-arrangement";
    constexpr int designFormatVersion = 2;
    constexpr auto setupFormat = "vmpc2000xl-arrangement-setup";
    constexpr int setupFormatVersion = 1;

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

    const char *anchorName(const AnchorAxis anchor)
    {
        switch (anchor)
        {
            case AnchorAxis::start:
                return "start";
            case AnchorAxis::end:
                return "end";
            case AnchorAxis::centre:
            default:
                return "centre";
        }
    }

    AnchorAxis parseAnchor(const json &value)
    {
        const auto name = value.get<std::string>();
        if (name == "start")
        {
            return AnchorAxis::start;
        }
        if (name == "centre")
        {
            return AnchorAxis::centre;
        }
        if (name == "end")
        {
            return AnchorAxis::end;
        }
        throw std::runtime_error("anchor must be start, centre, or end");
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

    json itemToJson(const ArrangementItemModel &item)
    {
        return {{"id", item.id},
                {"component", item.catalogId},
                {"position", pointToJson(item.position)},
                {"scale", item.scale},
                {"referenceSize", sizeToJson(item.referenceSize)}};
    }

    ArrangementItemModel parseItem(const json &value)
    {
        ArrangementItemModel result;
        result.id = value.at("id").get<std::uint64_t>();
        result.catalogId = value.at("component").get<std::string>();
        result.position = parsePoint(value.at("position"));
        result.scale = value.at("scale").get<float>();
        result.referenceSize = parseSize(value.at("referenceSize"));
        return result;
    }

    json nodeToJson(const ArrangementNodeModel &node)
    {
        json result{{"id", node.id},
                    {"type", node.isGroup() ? "group" : "component"},
                    {"anchorPosition", pointToJson(node.anchorPosition)},
                    {"widthFraction", node.widthFraction},
                    {"referenceSize", sizeToJson(node.referenceSize)},
                    {"anchor",
                     {{"horizontal", anchorName(node.anchor.horizontal)},
                      {"vertical", anchorName(node.anchor.vertical)}}}};
        if (node.isGroup())
        {
            result["children"] = json::array();
            for (const auto &child : node.children)
            {
                result["children"].push_back(itemToJson(child));
            }
        }
        else
        {
            result["component"] = node.catalogId;
        }
        return result;
    }

    ArrangementNodeModel parseNodeV2(const json &value)
    {
        ArrangementNodeModel result;
        result.id = value.at("id").get<std::uint64_t>();
        result.anchorPosition = parsePoint(value.at("anchorPosition"));
        result.widthFraction = value.at("widthFraction").get<float>();
        result.referenceSize = parseSize(value.at("referenceSize"));
        const auto &anchor = value.at("anchor");
        result.anchor = {parseAnchor(anchor.at("horizontal")),
                         parseAnchor(anchor.at("vertical"))};

        const auto type = value.at("type").get<std::string>();
        if (type == "component")
        {
            result.catalogId = value.at("component").get<std::string>();
        }
        else if (type == "group")
        {
            const auto &children = value.at("children");
            if (!children.is_array())
            {
                throw std::runtime_error("group children must be an array");
            }
            for (const auto &child : children)
            {
                result.children.push_back(parseItem(child));
            }
        }
        else
        {
            throw std::runtime_error("node type must be component or group");
        }
        return result;
    }

    float anchorFactor(const AnchorAxis axis)
    {
        switch (axis)
        {
            case AnchorAxis::start:
                return 0.f;
            case AnchorAxis::end:
                return 1.f;
            case AnchorAxis::centre:
            default:
                return 0.5f;
        }
    }

    ArrangementNodeModel parseNodeV1(const json &value,
                                     const LogicalSize referenceSize)
    {
        ArrangementNodeModel result;
        result.id = value.at("id").get<std::uint64_t>();
        const auto position = parsePoint(value.at("position"));
        const auto scale = value.at("scale").get<float>();
        result.referenceSize = parseSize(value.at("referenceSize"));
        const auto &anchor = value.at("anchor");
        result.anchor = {parseAnchor(anchor.at("horizontal")),
                         parseAnchor(anchor.at("vertical"))};
        const LogicalSize size{result.referenceSize.width * scale,
                               result.referenceSize.height * scale};
        result.anchorPosition = normalizedAnchorPosition(
            position, size, result.anchor, referenceSize);
        result.widthFraction =
            referenceSize.width > 0.f ? size.width / referenceSize.width : 0.f;

        const auto type = value.at("type").get<std::string>();
        if (type == "component")
        {
            result.catalogId = value.at("component").get<std::string>();
        }
        else if (type == "group")
        {
            for (const auto &child : value.at("children"))
            {
                result.children.push_back(parseItem(child));
            }
        }
        else
        {
            throw std::runtime_error("node type must be component or group");
        }
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
        const auto validateItem =
            [&ids, &error](const std::uint64_t id, const std::string &catalogId,
                           const LogicalPoint position, const float scale,
                           const LogicalSize referenceSize)
        {
            if (id == 0 || id == std::numeric_limits<std::uint64_t>::max() ||
                !ids.insert(id).second)
            {
                error =
                    "The design contains an invalid or duplicate component ID.";
                return false;
            }
            if (catalogId.empty())
            {
                error = "The design contains a component without a type.";
                return false;
            }
            if (!validPoint(position) || !std::isfinite(scale) ||
                scale <= 0.f || !validSize(referenceSize))
            {
                error = "The design contains invalid component geometry.";
                return false;
            }
            return true;
        };

        for (const auto &node : document.nodes)
        {
            const auto nodeType =
                node.isGroup() ? std::string("group") : node.catalogId;
            if (node.id == 0 ||
                node.id == std::numeric_limits<std::uint64_t>::max() ||
                !ids.insert(node.id).second || nodeType.empty())
            {
                error = "The design contains an invalid component ID or type.";
                return false;
            }
            if (!validPoint(node.anchorPosition) ||
                node.anchorPosition.x < 0.f || node.anchorPosition.x > 1.f ||
                node.anchorPosition.y < 0.f || node.anchorPosition.y > 1.f ||
                !std::isfinite(node.widthFraction) ||
                node.widthFraction <= 0.f || node.widthFraction > 1.f ||
                !validSize(node.referenceSize))
            {
                error = "The design contains invalid normalized geometry.";
                return false;
            }
            for (const auto &child : node.children)
            {
                if (!validateItem(child.id, child.catalogId, child.position,
                                  child.scale, child.referenceSize))
                {
                    return false;
                }
                const auto childRight =
                    child.position.x + child.referenceSize.width * child.scale;
                const auto childBottom =
                    child.position.y + child.referenceSize.height * child.scale;
                constexpr float epsilon = 0.001f;
                if (child.position.x < -epsilon ||
                    child.position.y < -epsilon ||
                    childRight > node.referenceSize.width + epsilon ||
                    childBottom > node.referenceSize.height + epsilon)
                {
                    error =
                        "The design contains a group child outside its group.";
                    return false;
                }
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
    AnchorAxis inferAnchorAxis(const float centre, const float extent)
    {
        if (centre < extent / 3.f)
        {
            return AnchorAxis::start;
        }
        if (centre > extent * 2.f / 3.f)
        {
            return AnchorAxis::end;
        }
        return AnchorAxis::centre;
    }

    bool responsiveLayoutIsValid(const ResponsiveLayout &layout,
                                 const LogicalSize targetSize)
    {
        if (!layout.hasValidPlacement)
        {
            return false;
        }
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

    ResponsiveLayout reflowProjectedLayout(ResponsiveLayout layout,
                                           const LogicalSize targetSize)
    {
        std::vector<size_t> placementOrder(layout.nodes.size());
        for (size_t i = 0; i < placementOrder.size(); ++i)
        {
            placementOrder[i] = i;
        }
        std::stable_sort(
            placementOrder.begin(), placementOrder.end(),
            [&layout](const auto firstIndex, const auto secondIndex)
            {
                const auto &first = layout.nodes[firstIndex].geometry.size;
                const auto &second = layout.nodes[secondIndex].geometry.size;
                return first.width * first.height >
                       second.width * second.height;
            });

        std::vector<LogicalRect> placedRectangles;
        placedRectangles.reserve(layout.nodes.size());
        for (const auto index : placementOrder)
        {
            auto &geometry = layout.nodes[index].geometry;
            const auto idealPosition = geometry.position;
            const auto placedPosition = findNearestAvailablePosition(
                idealPosition, geometry.size, targetSize, placedRectangles);
            if (!placedPosition.has_value())
            {
                layout.hasValidPlacement = false;
                return layout;
            }

            geometry.position = *placedPosition;
            geometry.reflowOffset = {placedPosition->x - idealPosition.x,
                                     placedPosition->y - idealPosition.y};
            placedRectangles.push_back({geometry.position, geometry.size});
        }
        layout.hasValidPlacement = true;
        return layout;
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
    const float sharedScale)
{
    if (node.referenceSize.width <= 0.f || targetSize.width <= 0.f ||
        targetSize.height <= 0.f)
    {
        return {};
    }
    const auto width = node.widthFraction * targetSize.width * sharedScale;
    const auto projectedScale = width / node.referenceSize.width;
    const LogicalSize size{width, node.referenceSize.height * projectedScale};
    const LogicalPoint projectedPivot{node.anchorPosition.x * targetSize.width,
                                      node.anchorPosition.y *
                                          targetSize.height};
    return {
        {projectedPivot.x - size.width * anchorFactor(node.anchor.horizontal),
         projectedPivot.y - size.height * anchorFactor(node.anchor.vertical)},
        size,
        projectedScale,
        {}};
}

LogicalPoint vmpc_juce::gui::arrangement::normalizedAnchorPosition(
    const LogicalPoint projectedPosition, const LogicalSize projectedSize,
    const ArrangementAnchor anchor, const LogicalSize targetSize)
{
    if (targetSize.width <= 0.f || targetSize.height <= 0.f)
    {
        return {};
    }
    return {(projectedPosition.x +
             projectedSize.width * anchorFactor(anchor.horizontal)) /
                targetSize.width,
            (projectedPosition.y +
             projectedSize.height * anchorFactor(anchor.vertical)) /
                targetSize.height};
}

ResponsiveLayout vmpc_juce::gui::arrangement::projectDocumentAtScale(
    const ArrangementDocument &document, const LogicalSize targetSize,
    const float sharedScale)
{
    ResponsiveLayout result;
    result.sharedScale = std::max(0.f, sharedScale);
    result.nodes.reserve(document.nodes.size());
    for (const auto &node : document.nodes)
    {
        result.nodes.push_back(
            {node.id,
             projectNodeAtScale(node, targetSize, result.sharedScale)});
    }
    return result;
}

ResponsiveLayout vmpc_juce::gui::arrangement::computeResponsiveLayout(
    const ArrangementDocument &document, const LogicalSize targetSize)
{
    if (document.nodes.empty() || targetSize.width <= 0.f ||
        targetSize.height <= 0.f)
    {
        return projectDocumentAtScale(document, targetSize, 1.f);
    }

    constexpr float upperScale = 1.f;
    auto upperLayout = reflowProjectedLayout(
        projectDocumentAtScale(document, targetSize, upperScale), targetSize);
    if (responsiveLayoutIsValid(upperLayout, targetSize))
    {
        return upperLayout;
    }

    auto lower = 0.f;
    auto upper = upperScale;
    for (int iteration = 0; iteration < 40; ++iteration)
    {
        const auto candidate = (lower + upper) * 0.5f;
        const auto layout = reflowProjectedLayout(
            projectDocumentAtScale(document, targetSize, candidate),
            targetSize);
        if (responsiveLayoutIsValid(layout, targetSize))
        {
            lower = candidate;
        }
        else
        {
            upper = candidate;
        }
    }
    return reflowProjectedLayout(
        projectDocumentAtScale(document, targetSize, lower), targetSize);
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
    constexpr float epsilon = 0.001f;
    return first.position.x + first.size.width > second.position.x + epsilon &&
           second.position.x + second.size.width > first.position.x + epsilon &&
           first.position.y + first.size.height > second.position.y + epsilon &&
           second.position.y + second.size.height > first.position.y + epsilon;
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

std::string vmpc_juce::gui::arrangement::serializeArrangementDocument(
    const ArrangementDocument &document)
{
    json result{{"format", designFormat},
                {"version", designFormatVersion},
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
        if (format != designFormat && format != legacyDesignFormat)
        {
            throw std::runtime_error("not a VMPC2000XL arrangement");
        }
        const auto version = source.at("version").get<int>();
        if (version != 1 && version != designFormatVersion)
        {
            throw std::runtime_error("unsupported design version");
        }

        ArrangementDocument result;
        const auto &nodes = source.at("nodes");
        if (!nodes.is_array())
        {
            throw std::runtime_error("nodes must be an array");
        }
        if (version == designFormatVersion)
        {
            for (const auto &node : nodes)
            {
                result.nodes.push_back(parseNodeV2(node));
            }
        }
        else
        {
            const auto referenceSize =
                parseSize(source.at("reference").at("size"));
            if (!validSize(referenceSize))
            {
                throw std::runtime_error("invalid legacy reference size");
            }
            for (const auto &node : nodes)
            {
                result.nodes.push_back(parseNodeV1(node, referenceSize));
            }
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
    for (const auto &slot : setup.slots)
    {
        if (!slot.has_value())
        {
            slots.push_back(nullptr);
            continue;
        }
        slots.push_back(
            {{"orientation", orientationName(slot->orientation)},
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
        if (source.at("version").get<int>() != setupFormatVersion)
        {
            throw std::runtime_error("unsupported setup version");
        }
        const auto &slots = source.at("slots");
        if (!slots.is_array() || slots.size() != ArrangementSetup::slotCount)
        {
            throw std::runtime_error("setup must contain exactly five slots");
        }

        ArrangementSetup result;
        for (std::size_t i = 0; i < ArrangementSetup::slotCount; ++i)
        {
            const auto &sourceSlot = slots.at(i);
            if (sourceSlot.is_null())
            {
                continue;
            }
            ArrangementSlot slot;
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

ArrangementAnchor
vmpc_juce::gui::arrangement::inferAnchor(const LogicalPoint position,
                                         const LogicalSize size,
                                         const LogicalSize canvasSize)
{
    return {
        inferAnchorAxis(position.x + size.width * 0.5f, canvasSize.width),
        inferAnchorAxis(position.y + size.height * 0.5f, canvasSize.height)};
}

ArrangementNodeModel vmpc_juce::gui::arrangement::makeFixedGroup(
    const std::uint64_t groupId,
    const std::vector<ArrangementNodeModel> &itemNodes,
    const ResponsiveLayout &layout, const LogicalSize targetSize)
{
    ArrangementNodeModel group;
    group.id = groupId;
    if (itemNodes.empty() || targetSize.width <= 0.f)
    {
        return group;
    }

    const auto *firstGeometry =
        findProjectedGeometry(layout, itemNodes.front().id);
    if (firstGeometry == nullptr)
    {
        return group;
    }
    auto left = firstGeometry->position.x;
    auto top = firstGeometry->position.y;
    auto right = left + firstGeometry->size.width;
    auto bottom = top + firstGeometry->size.height;
    for (const auto &node : itemNodes)
    {
        const auto *geometry = findProjectedGeometry(layout, node.id);
        if (geometry == nullptr)
        {
            return {};
        }
        left = std::min(left, geometry->position.x);
        top = std::min(top, geometry->position.y);
        right = std::max(right, geometry->position.x + geometry->size.width);
        bottom = std::max(bottom, geometry->position.y + geometry->size.height);
    }

    const LogicalPoint displayedPosition{left, top};
    const LogicalSize displayedSize{right - left, bottom - top};
    group.referenceSize = displayedSize;
    group.widthFraction = group.referenceSize.width / targetSize.width;
    group.anchor = inferAnchor(displayedPosition, displayedSize, targetSize);
    group.anchorPosition = normalizedAnchorPosition(
        displayedPosition, displayedSize, group.anchor, targetSize);
    group.children.reserve(itemNodes.size());
    for (const auto &node : itemNodes)
    {
        const auto *geometry = findProjectedGeometry(layout, node.id);
        group.children.push_back(
            {node.id,
             node.catalogId,
             {geometry->position.x - left, geometry->position.y - top},
             geometry->scale,
             node.referenceSize});
    }
    return group;
}

std::vector<ArrangementNodeModel>
vmpc_juce::gui::arrangement::ungroupFixedGroup(
    const ArrangementNodeModel &group,
    const ProjectedNodeGeometry groupGeometry, const LogicalSize targetSize)
{
    std::vector<ArrangementNodeModel> result;
    if (targetSize.width <= 0.f)
    {
        return result;
    }
    result.reserve(group.children.size());
    for (const auto &child : group.children)
    {
        ArrangementNodeModel node;
        node.id = child.id;
        node.catalogId = child.catalogId;
        node.referenceSize = child.referenceSize;
        const LogicalPoint displayedPosition{
            groupGeometry.position.x + child.position.x * groupGeometry.scale,
            groupGeometry.position.y + child.position.y * groupGeometry.scale};
        const LogicalSize displayedSize{
            child.referenceSize.width * child.scale * groupGeometry.scale,
            child.referenceSize.height * child.scale * groupGeometry.scale};
        node.widthFraction = displayedSize.width / targetSize.width;
        node.anchor = inferAnchor(displayedPosition, displayedSize, targetSize);
        node.anchorPosition = normalizedAnchorPosition(
            displayedPosition, displayedSize, node.anchor, targetSize);
        result.push_back(std::move(node));
    }
    return result;
}
