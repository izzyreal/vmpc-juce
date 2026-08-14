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

TEST_CASE("GUI Lab responsive projection uses uniform fit and 3 by 3 anchors",
          "[gui-lab][arrangement][responsive]")
{
    const auto transform =
        makeResponsiveTransform({100.f, 200.f}, {400.f, 300.f});
    CHECK(transform.scale == Catch::Approx(1.5f));
    CHECK(transform.horizontalSlack == Catch::Approx(250.f));
    CHECK(transform.verticalSlack == Catch::Approx(0.f));

    CHECK(projectPosition({10.f, 20.f},
                          {AnchorAxis::start, AnchorAxis::start}, transform)
              .x == Catch::Approx(15.f));
    CHECK(projectPosition({10.f, 20.f},
                          {AnchorAxis::centre, AnchorAxis::start}, transform)
              .x == Catch::Approx(140.f));
    CHECK(projectPosition({10.f, 20.f},
                          {AnchorAxis::end, AnchorAxis::start}, transform)
              .x == Catch::Approx(265.f));
}

TEST_CASE("GUI Lab responsive coordinates round trip without changing the node",
          "[gui-lab][arrangement][responsive]")
{
    ArrangementNodeModel node;
    node.id = 7;
    node.catalogId = "cursor";
    node.anchor = {AnchorAxis::end, AnchorAxis::centre};
    node.position = {61.f, 89.f};
    node.scale = 1.25f;
    node.referenceSize = {48.f, 48.f};
    const auto original = node;
    const auto transform =
        makeResponsiveTransform({390.f, 844.f}, {844.f, 390.f});

    const auto projected = projectNode(node, transform);
    const auto roundTrip =
        unprojectPosition(projected.position, node.anchor, transform);
    CHECK(roundTrip.x == Catch::Approx(original.position.x));
    CHECK(roundTrip.y == Catch::Approx(original.position.y));
    CHECK(projected.scale == Catch::Approx(original.scale * transform.scale));
    CHECK(node.id == original.id);
    CHECK(node.catalogId == original.catalogId);
    CHECK(node.position.x == original.position.x);
    CHECK(node.position.y == original.position.y);
    CHECK(node.scale == original.scale);
}

TEST_CASE("GUI Lab fixed groups preserve child geometry when ungrouped",
          "[gui-lab][arrangement][responsive]")
{
    ArrangementNodeModel first;
    first.id = 1;
    first.catalogId = "cursor";
    first.position = {10.f, 20.f};
    first.scale = 2.f;
    first.referenceSize = {20.f, 10.f};

    ArrangementNodeModel second;
    second.id = 2;
    second.catalogId = "data-wheel";
    second.position = {60.f, 15.f};
    second.scale = 1.f;
    second.referenceSize = {10.f, 30.f};

    const auto group = makeFixedGroup(9, {first, second}, {100.f, 200.f});
    CHECK(group.isGroup());
    CHECK(group.position.x == Catch::Approx(10.f));
    CHECK(group.position.y == Catch::Approx(15.f));
    CHECK(group.referenceSize.width == Catch::Approx(60.f));
    CHECK(group.referenceSize.height == Catch::Approx(30.f));
    REQUIRE(group.children.size() == 2);
    CHECK(group.children[0].position.x == Catch::Approx(0.f));
    CHECK(group.children[0].position.y == Catch::Approx(5.f));
    CHECK(group.children[1].position.x == Catch::Approx(50.f));
    CHECK(group.children[1].position.y == Catch::Approx(0.f));

    const auto children = ungroupFixedGroup(group, {100.f, 200.f});
    REQUIRE(children.size() == 2);
    CHECK(children[0].position.x == Catch::Approx(first.position.x));
    CHECK(children[0].position.y == Catch::Approx(first.position.y));
    CHECK(children[0].scale == Catch::Approx(first.scale));
    CHECK(children[1].position.x == Catch::Approx(second.position.x));
    CHECK(children[1].position.y == Catch::Approx(second.position.y));
    CHECK(children[1].scale == Catch::Approx(second.scale));
}

TEST_CASE("GUI Lab responsive layout uses the largest safe common scale",
          "[gui-lab][arrangement][responsive]")
{
    ArrangementDocument document;
    document.referenceSize = {100.f, 200.f};

    ArrangementNodeModel fullWidth;
    fullWidth.id = 1;
    fullWidth.anchor = {AnchorAxis::centre, AnchorAxis::start};
    fullWidth.referenceSize = {100.f, 20.f};
    document.nodes = {fullWidth};

    const auto referenceLayout =
        computeResponsiveLayout(document, document.referenceSize);
    CHECK(referenceLayout.sharedScale == Catch::Approx(1.f));
    REQUIRE(referenceLayout.nodes.size() == 1);
    CHECK(referenceLayout.nodes.front().geometry.size.width ==
          Catch::Approx(100.f));

    const auto shorterTarget =
        computeResponsiveLayout(document, {80.f, 100.f});
    CHECK(shorterTarget.sharedScale == Catch::Approx(0.8f));
    CHECK(shorterTarget.nodes.front().geometry.position.x ==
          Catch::Approx(0.f));
    CHECK(shorterTarget.nodes.front().geometry.size.width ==
          Catch::Approx(80.f));
}

TEST_CASE("GUI Lab responsive layout reflows a projected component collision",
          "[gui-lab][arrangement][responsive]")
{
    ArrangementDocument document;
    document.referenceSize = {100.f, 200.f};

    ArrangementNodeModel left;
    left.id = 1;
    left.anchor = {AnchorAxis::start, AnchorAxis::start};
    left.position = {0.f, 0.f};
    left.referenceSize = {40.f, 20.f};
    ArrangementNodeModel right = left;
    right.id = 2;
    right.position = {50.f, 0.f};
    document.nodes = {left, right};

    const auto layout = computeResponsiveLayout(document, {80.f, 100.f});
    CHECK(layout.sharedScale == Catch::Approx(0.8f).margin(0.0001f));
    REQUIRE(layout.nodes.size() == 2);
    const auto first = layout.nodes[0].geometry;
    const auto second = layout.nodes[1].geometry;
    CHECK_FALSE(rectanglesOverlap({first.position, first.size},
                                  {second.position, second.size}));
    CHECK(layout.nodes[0].geometry.reflowOffset.x == Catch::Approx(0.f));
    CHECK(layout.nodes[0].geometry.reflowOffset.y == Catch::Approx(0.f));
    CHECK((layout.nodes[0].geometry.reflowOffset.x != 0.f ||
           layout.nodes[0].geometry.reflowOffset.y != 0.f ||
           layout.nodes[1].geometry.reflowOffset.x != 0.f ||
           layout.nodes[1].geometry.reflowOffset.y != 0.f));
}

TEST_CASE("GUI Lab target reflow prevents zero scale on tall Max iPhones",
          "[gui-lab][arrangement][responsive]")
{
    ArrangementDocument document;
    document.referenceSize = {320.f, 480.f};

    ArrangementNodeModel upper;
    upper.id = 1;
    upper.anchor = {AnchorAxis::centre, AnchorAxis::centre};
    upper.position = {50.f, 43.7198f};
    upper.referenceSize = {100.f, 100.f};

    ArrangementNodeModel lower;
    lower.id = 2;
    lower.anchor = {AnchorAxis::centre, AnchorAxis::start};
    lower.position = {50.f, 200.f};
    lower.referenceSize = {100.f, 50.f};
    document.nodes = {upper, lower};
    const auto originalUpper = upper.position;
    const auto originalLower = lower.position;

    for (const auto target : {LogicalSize{414.f, 896.f},
                              LogicalSize{428.f, 926.f}})
    {
        const auto layout = computeResponsiveLayout(document, target);
        CHECK(layout.hasValidPlacement);
        CHECK(layout.sharedScale > 1.f);
        REQUIRE(layout.nodes.size() == 2);
        const auto first = layout.nodes[0].geometry;
        const auto second = layout.nodes[1].geometry;
        CHECK_FALSE(rectanglesOverlap({first.position, first.size},
                                      {second.position, second.size}));
        CHECK((first.reflowOffset.x != 0.f || first.reflowOffset.y != 0.f ||
               second.reflowOffset.x != 0.f || second.reflowOffset.y != 0.f));
    }

    CHECK(document.nodes[0].position.x == originalUpper.x);
    CHECK(document.nodes[0].position.y == originalUpper.y);
    CHECK(document.nodes[1].position.x == originalLower.x);
    CHECK(document.nodes[1].position.y == originalLower.y);
}

TEST_CASE("GUI Lab document placement finds the nearest non-overlapping gap",
          "[gui-lab][arrangement][collision]")
{
    ArrangementDocument document;
    document.referenceSize = {100.f, 100.f};
    ArrangementNodeModel obstacle;
    obstacle.id = 1;
    obstacle.position = {40.f, 0.f};
    obstacle.referenceSize = {20.f, 100.f};
    document.nodes = {obstacle};

    ArrangementNodeModel candidate;
    candidate.id = 2;
    candidate.referenceSize = {20.f, 20.f};
    const auto nearest = findNearestValidPosition(
        document, candidate, {45.f, 30.f}, false);
    REQUIRE(nearest.has_value());
    CHECK(nearest->x == Catch::Approx(60.f));
    CHECK(nearest->y == Catch::Approx(30.f));

    candidate.position = *nearest;
    CHECK(isNodePlacementValid(document, candidate));
    CHECK_FALSE(rectanglesOverlap({{20.f, 0.f}, {20.f, 20.f}},
                                  getNodeRect(obstacle)));
    CHECK(rectanglesOverlap({{20.01f, 0.f}, {20.f, 20.f}},
                            getNodeRect(obstacle)));

    obstacle.position = {0.f, 0.f};
    obstacle.referenceSize = {100.f, 100.f};
    document.nodes = {obstacle};
    CHECK_FALSE(findNearestValidPosition(document, candidate, {0.f, 0.f},
                                         false)
                    .has_value());
}

TEST_CASE("GUI Lab corner resizing preserves the selected pivot",
          "[gui-lab][arrangement][resize]")
{
    const ProjectedNodeGeometry start{{10.f, 20.f}, {100.f, 50.f}, 1.f};
    const LogicalSize larger{120.f, 60.f};

    const auto topLeft = positionForResizedNode(
        start, larger, ResizeCorner::topLeft, false);
    CHECK(topLeft.x == Catch::Approx(-10.f));
    CHECK(topLeft.y == Catch::Approx(10.f));
    const auto topRight = positionForResizedNode(
        start, larger, ResizeCorner::topRight, false);
    CHECK(topRight.x == Catch::Approx(10.f));
    CHECK(topRight.y == Catch::Approx(10.f));
    const auto bottomLeft = positionForResizedNode(
        start, larger, ResizeCorner::bottomLeft, false);
    CHECK(bottomLeft.x == Catch::Approx(-10.f));
    CHECK(bottomLeft.y == Catch::Approx(20.f));
    const auto bottomRight = positionForResizedNode(
        start, larger, ResizeCorner::bottomRight, false);
    CHECK(bottomRight.x == Catch::Approx(10.f));
    CHECK(bottomRight.y == Catch::Approx(20.f));

    const auto centred = positionForResizedNode(
        start, larger, ResizeCorner::topLeft, true);
    CHECK(centred.x == Catch::Approx(0.f));
    CHECK(centred.y == Catch::Approx(15.f));
}

TEST_CASE("GUI Lab resize inversion removes preview-only reflow offsets",
          "[gui-lab][arrangement][responsive][resize]")
{
    ArrangementNodeModel node;
    node.anchor = {AnchorAxis::centre, AnchorAxis::centre};
    node.position = {10.f, 20.f};
    node.referenceSize = {20.f, 10.f};
    const auto transform =
        makeResponsiveTransform({100.f, 200.f}, {80.f, 100.f});
    constexpr float sharedScale = 0.8f;
    auto start = projectNodeAtScale(node, transform, sharedScale);
    start.reflowOffset = {5.f, 7.f};
    start.position.x += start.reflowOffset.x;
    start.position.y += start.reflowOffset.y;

    constexpr float requestedProjectedScale = 1.2f;
    const LogicalSize requestedSize{
        node.referenceSize.width * requestedProjectedScale,
        node.referenceSize.height * requestedProjectedScale};
    const auto displayedPosition = positionForResizedNode(
        start, requestedSize, ResizeCorner::topLeft, false);
    const LogicalPoint idealPosition{
        displayedPosition.x - start.reflowOffset.x,
        displayedPosition.y - start.reflowOffset.y};

    auto resized = node;
    resized.scale = requestedProjectedScale / sharedScale;
    resized.position = unprojectNodePosition(
        resized, idealPosition, transform, sharedScale);
    auto projected =
        projectNodeAtScale(resized, transform, sharedScale);
    projected.position.x += start.reflowOffset.x;
    projected.position.y += start.reflowOffset.y;

    CHECK(projected.position.x == Catch::Approx(displayedPosition.x));
    CHECK(projected.position.y == Catch::Approx(displayedPosition.y));
    CHECK(projected.position.x + projected.size.width ==
          Catch::Approx(start.position.x + start.size.width));
    CHECK(projected.position.y + projected.size.height ==
          Catch::Approx(start.position.y + start.size.height));
}
