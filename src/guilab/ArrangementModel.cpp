#include "ArrangementModel.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

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

namespace
{
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

    LogicalPoint nodePivot(const ArrangementNodeModel &node)
    {
        const auto size = LogicalSize{node.referenceSize.width * node.scale,
                                      node.referenceSize.height * node.scale};
        return {node.position.x +
                    size.width * anchorFactor(node.anchor.horizontal),
                node.position.y +
                    size.height * anchorFactor(node.anchor.vertical)};
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
            geometry.reflowOffset = {
                placedPosition->x - idealPosition.x,
                placedPosition->y - idealPosition.y};
            placedRectangles.push_back({geometry.position, geometry.size});
        }
        layout.hasValidPlacement = true;
        return layout;
    }
} // namespace

ResponsiveTransform vmpc_juce::guilab::makeResponsiveTransform(
    const LogicalSize referenceSize, const LogicalSize targetSize)
{
    if (referenceSize.width <= 0.f || referenceSize.height <= 0.f ||
        targetSize.width <= 0.f || targetSize.height <= 0.f)
    {
        return {};
    }

    ResponsiveTransform result;
    result.scale = std::min(targetSize.width / referenceSize.width,
                            targetSize.height / referenceSize.height);
    result.horizontalSlack =
        std::max(0.f, targetSize.width - referenceSize.width * result.scale);
    result.verticalSlack =
        std::max(0.f, targetSize.height - referenceSize.height * result.scale);
    return result;
}

LogicalPoint vmpc_juce::guilab::projectPosition(
    const LogicalPoint canonicalPosition, const ArrangementAnchor anchor,
    const ResponsiveTransform transform)
{
    return {canonicalPosition.x * transform.scale +
                anchorFactor(anchor.horizontal) * transform.horizontalSlack,
            canonicalPosition.y * transform.scale +
                anchorFactor(anchor.vertical) * transform.verticalSlack};
}

LogicalPoint vmpc_juce::guilab::unprojectPosition(
    const LogicalPoint projectedPosition, const ArrangementAnchor anchor,
    const ResponsiveTransform transform)
{
    const auto inverseScale = transform.scale > 0.f ? 1.f / transform.scale : 0.f;
    return {(projectedPosition.x -
             anchorFactor(anchor.horizontal) * transform.horizontalSlack) *
                inverseScale,
            (projectedPosition.y -
             anchorFactor(anchor.vertical) * transform.verticalSlack) *
                inverseScale};
}

ProjectedNodeGeometry vmpc_juce::guilab::projectNode(
    const ArrangementNodeModel &node, const ResponsiveTransform transform)
{
    return projectNodeAtScale(node, transform, transform.scale);
}

ProjectedNodeGeometry vmpc_juce::guilab::projectNodeAtScale(
    const ArrangementNodeModel &node, const ResponsiveTransform transform,
    const float sharedScale)
{
    const auto canonicalPivot = nodePivot(node);
    const LogicalPoint projectedPivot{
        canonicalPivot.x * transform.scale +
            anchorFactor(node.anchor.horizontal) * transform.horizontalSlack,
        canonicalPivot.y * transform.scale +
            anchorFactor(node.anchor.vertical) * transform.verticalSlack};
    const auto projectedScale = node.scale * sharedScale;
    const LogicalSize size{node.referenceSize.width * projectedScale,
                           node.referenceSize.height * projectedScale};
    return {{projectedPivot.x -
                 size.width * anchorFactor(node.anchor.horizontal),
             projectedPivot.y -
                 size.height * anchorFactor(node.anchor.vertical)},
            size, projectedScale, {}};
}

LogicalPoint vmpc_juce::guilab::unprojectNodePosition(
    const ArrangementNodeModel &node, const LogicalPoint projectedPosition,
    const ResponsiveTransform transform, const float sharedScale)
{
    if (transform.scale <= 0.f)
    {
        return node.position;
    }

    const LogicalSize projectedSize{
        node.referenceSize.width * node.scale * sharedScale,
        node.referenceSize.height * node.scale * sharedScale};
    const LogicalPoint projectedPivot{
        projectedPosition.x +
            projectedSize.width * anchorFactor(node.anchor.horizontal),
        projectedPosition.y +
            projectedSize.height * anchorFactor(node.anchor.vertical)};
    const LogicalPoint canonicalPivot{
        (projectedPivot.x -
         anchorFactor(node.anchor.horizontal) * transform.horizontalSlack) /
            transform.scale,
        (projectedPivot.y -
         anchorFactor(node.anchor.vertical) * transform.verticalSlack) /
            transform.scale};
    const LogicalSize canonicalSize{
        node.referenceSize.width * node.scale,
        node.referenceSize.height * node.scale};
    return {canonicalPivot.x -
                canonicalSize.width * anchorFactor(node.anchor.horizontal),
            canonicalPivot.y -
                canonicalSize.height * anchorFactor(node.anchor.vertical)};
}

ResponsiveLayout vmpc_juce::guilab::projectDocumentAtScale(
    const ArrangementDocument &document, const LogicalSize targetSize,
    const float sharedScale)
{
    ResponsiveLayout result;
    result.transform =
        makeResponsiveTransform(document.referenceSize, targetSize);
    result.sharedScale = std::max(0.f, sharedScale);
    result.nodes.reserve(document.nodes.size());
    for (const auto &node : document.nodes)
    {
        result.nodes.push_back(
            {node.id,
             projectNodeAtScale(node, result.transform, result.sharedScale)});
    }
    return result;
}

ResponsiveLayout vmpc_juce::guilab::computeResponsiveLayout(
    const ArrangementDocument &document, const LogicalSize targetSize)
{
    const auto transform =
        makeResponsiveTransform(document.referenceSize, targetSize);
    if (document.nodes.empty() || document.referenceSize.width <= 0.f ||
        document.referenceSize.height <= 0.f)
    {
        return projectDocumentAtScale(document, targetSize, transform.scale);
    }

    const auto upperScale = std::max(
        targetSize.width / document.referenceSize.width,
        targetSize.height / document.referenceSize.height);
    auto upperLayout = reflowProjectedLayout(
        projectDocumentAtScale(document, targetSize, upperScale), targetSize);
    if (responsiveLayoutIsValid(upperLayout, targetSize))
    {
        return upperLayout;
    }

    auto lower = 0.f;
    auto upper = std::max(0.f, upperScale);
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

const ProjectedNodeGeometry *vmpc_juce::guilab::findProjectedGeometry(
    const ResponsiveLayout &layout, const std::uint64_t nodeId)
{
    const auto found = std::find_if(layout.nodes.begin(), layout.nodes.end(),
                                    [nodeId](const auto &node)
                                    {
                                        return node.id == nodeId;
                                    });
    return found == layout.nodes.end() ? nullptr : &found->geometry;
}

LogicalRect
vmpc_juce::guilab::getNodeRect(const ArrangementNodeModel &node)
{
    return {node.position,
            {node.referenceSize.width * node.scale,
             node.referenceSize.height * node.scale}};
}

bool vmpc_juce::guilab::rectanglesOverlap(const LogicalRect first,
                                          const LogicalRect second)
{
    constexpr float epsilon = 0.001f;
    return first.position.x + first.size.width > second.position.x + epsilon &&
           second.position.x + second.size.width > first.position.x + epsilon &&
           first.position.y + first.size.height > second.position.y + epsilon &&
           second.position.y + second.size.height > first.position.y + epsilon;
}

std::optional<LogicalPoint>
vmpc_juce::guilab::findNearestAvailablePosition(
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

bool vmpc_juce::guilab::isNodePlacementValid(
    const ArrangementDocument &document,
    const ArrangementNodeModel &candidate,
    const std::uint64_t ignoredNodeId)
{
    constexpr float epsilon = 0.001f;
    const auto candidateRect = getNodeRect(candidate);
    if (candidateRect.position.x < -epsilon ||
        candidateRect.position.y < -epsilon ||
        candidateRect.position.x + candidateRect.size.width >
            document.referenceSize.width + epsilon ||
        candidateRect.position.y + candidateRect.size.height >
            document.referenceSize.height + epsilon)
    {
        return false;
    }

    return std::none_of(document.nodes.begin(), document.nodes.end(),
                        [&](const auto &node)
                        {
                            return node.id != ignoredNodeId &&
                                   rectanglesOverlap(candidateRect,
                                                     getNodeRect(node));
                        });
}

std::optional<LogicalPoint> vmpc_juce::guilab::findNearestValidPosition(
    const ArrangementDocument &document,
    const ArrangementNodeModel &candidate,
    const LogicalPoint requestedPosition, const bool shouldSnapToGrid,
    const std::uint64_t ignoredNodeId, const float gridSize)
{
    std::vector<LogicalRect> obstacles;
    obstacles.reserve(document.nodes.size());
    for (const auto &node : document.nodes)
    {
        if (node.id != ignoredNodeId)
        {
            obstacles.push_back(getNodeRect(node));
        }
    }
    return findNearestAvailablePosition(
        requestedPosition, getNodeRect(candidate).size,
        document.referenceSize, obstacles, shouldSnapToGrid, gridSize);
}

LogicalPoint vmpc_juce::guilab::positionForResizedNode(
    const ProjectedNodeGeometry startGeometry,
    const LogicalSize requestedSize, const ResizeCorner corner,
    const bool useCentrePivot)
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
        return {startGeometry.position.x,
                startGeometry.position.y + startGeometry.size.height -
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

ArrangementAnchor vmpc_juce::guilab::inferAnchor(
    const LogicalPoint position, const LogicalSize size,
    const LogicalSize canvasSize)
{
    return {inferAnchorAxis(position.x + size.width * 0.5f, canvasSize.width),
            inferAnchorAxis(position.y + size.height * 0.5f,
                            canvasSize.height)};
}

ArrangementNodeModel vmpc_juce::guilab::makeFixedGroup(
    const std::uint64_t groupId,
    const std::vector<ArrangementNodeModel> &itemNodes,
    const LogicalSize referenceCanvasSize)
{
    ArrangementNodeModel group;
    group.id = groupId;
    if (itemNodes.empty())
    {
        return group;
    }

    auto left = itemNodes.front().position.x;
    auto top = itemNodes.front().position.y;
    auto right = left + itemNodes.front().referenceSize.width *
                            itemNodes.front().scale;
    auto bottom = top + itemNodes.front().referenceSize.height *
                              itemNodes.front().scale;
    for (const auto &node : itemNodes)
    {
        left = std::min(left, node.position.x);
        top = std::min(top, node.position.y);
        right = std::max(right, node.position.x +
                                    node.referenceSize.width * node.scale);
        bottom = std::max(bottom, node.position.y +
                                      node.referenceSize.height * node.scale);
    }

    group.position = {left, top};
    group.referenceSize = {right - left, bottom - top};
    group.anchor = inferAnchor(group.position, group.referenceSize,
                               referenceCanvasSize);
    group.children.reserve(itemNodes.size());
    for (const auto &node : itemNodes)
    {
        group.children.push_back({node.id, node.catalogId,
                                  {node.position.x - left,
                                   node.position.y - top},
                                  node.scale, node.referenceSize});
    }
    return group;
}

std::vector<ArrangementNodeModel> vmpc_juce::guilab::ungroupFixedGroup(
    const ArrangementNodeModel &group, const LogicalSize referenceCanvasSize)
{
    std::vector<ArrangementNodeModel> result;
    result.reserve(group.children.size());
    for (const auto &child : group.children)
    {
        ArrangementNodeModel node;
        node.id = child.id;
        node.catalogId = child.catalogId;
        node.position = {group.position.x + child.position.x * group.scale,
                         group.position.y + child.position.y * group.scale};
        node.scale = child.scale * group.scale;
        node.referenceSize = child.referenceSize;
        node.anchor = inferAnchor(
            node.position,
            {node.referenceSize.width * node.scale,
             node.referenceSize.height * node.scale},
            referenceCanvasSize);
        result.push_back(std::move(node));
    }
    return result;
}
