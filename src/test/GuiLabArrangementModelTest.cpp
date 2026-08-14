#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "ArrangementModel.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <string>

using namespace vmpc_juce::guilab;

namespace
{
    ArrangementNodeModel makeNode(const std::uint64_t id,
                                  const LogicalPoint anchorPosition,
                                  const float widthFraction,
                                  const LogicalSize referenceSize,
                                  const ArrangementAnchor anchor = {})
    {
        ArrangementNodeModel node;
        node.id = id;
        node.catalogId = "cursor";
        node.anchor = anchor;
        node.anchorPosition = anchorPosition;
        node.widthFraction = widthFraction;
        node.referenceSize = referenceSize;
        return node;
    }

    void checkValidLayout(const ResponsiveLayout &layout,
                          const LogicalSize bounds)
    {
        REQUIRE(layout.hasValidPlacement);
        for (size_t i = 0; i < layout.nodes.size(); ++i)
        {
            const auto &first = layout.nodes[i].geometry;
            CHECK(first.position.x >= Catch::Approx(0.f).margin(0.001f));
            CHECK(first.position.y >= Catch::Approx(0.f).margin(0.001f));
            CHECK(first.position.x + first.size.width <=
                  Catch::Approx(bounds.width).margin(0.001f));
            CHECK(first.position.y + first.size.height <=
                  Catch::Approx(bounds.height).margin(0.001f));
            for (size_t j = i + 1; j < layout.nodes.size(); ++j)
            {
                const auto &second = layout.nodes[j].geometry;
                CHECK_FALSE(rectanglesOverlap(
                    {first.position, first.size},
                    {second.position, second.size}));
            }
        }
    }
} // namespace

TEST_CASE("GUI Lab compact LCD sizes follow the production grid",
          "[gui-lab][arrangement]")
{
    CHECK(compactDisplayReferenceSize.width == Catch::Approx(229.52338f));
    CHECK(compactDisplayReferenceSize.height == Catch::Approx(115.75869f));
    CHECK(compactMountedLcdReferenceSize.width ==
          compactDisplayReferenceSize.width);
    CHECK(compactMountedLcdReferenceSize.height ==
          Catch::Approx(88.19710f));
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
    CHECK(snapItemScaleToGrid(1.03f, {100.f, 40.f}) == 1.04f);
    CHECK(constrainItemScale(0.1f, reference, {390.f, 844.f}) == 0.5f);
    CHECK(constrainItemScale(3.f, reference, {390.f, 844.f}) ==
          390.f / 230.f);
}

TEST_CASE("GUI Lab normalized width and anchors project without a device",
          "[gui-lab][arrangement][responsive]")
{
    const LogicalSize viewport{400.f, 300.f};
    const auto node = makeNode(
        1, {0.25f, 0.5f}, 0.5f, {100.f, 50.f},
        {AnchorAxis::centre, AnchorAxis::end});
    const auto projected = projectNode(node, viewport);
    CHECK(projected.size.width == Catch::Approx(200.f));
    CHECK(projected.size.height == Catch::Approx(100.f));
    CHECK(projected.scale == Catch::Approx(2.f));
    CHECK(projected.position.x == Catch::Approx(0.f));
    CHECK(projected.position.y == Catch::Approx(50.f));

    const auto normalized = normalizedAnchorPosition(
        projected.position, projected.size, node.anchor, viewport);
    CHECK(normalized.x == Catch::Approx(node.anchorPosition.x));
    CHECK(normalized.y == Catch::Approx(node.anchorPosition.y));
}

TEST_CASE("GUI Lab width fraction one spans the available width",
          "[gui-lab][arrangement][responsive]")
{
    ArrangementDocument document;
    document.nodes = {makeNode(
        1, {0.5f, 0.f}, 1.f, {200.f, 50.f},
        {AnchorAxis::centre, AnchorAxis::start})};

    for (const auto viewport : {LogicalSize{320.f, 568.f},
                                LogicalSize{428.f, 926.f},
                                LogicalSize{844.f, 390.f}})
    {
        const auto layout = computeResponsiveLayout(document, viewport);
        REQUIRE(layout.nodes.size() == 1);
        CHECK(layout.sharedScale == Catch::Approx(1.f));
        CHECK(layout.nodes[0].geometry.position.x == Catch::Approx(0.f));
        CHECK(layout.nodes[0].geometry.size.width ==
              Catch::Approx(viewport.width));
    }
}

TEST_CASE("GUI Lab responsive layout compresses and reflows when necessary",
          "[gui-lab][arrangement][responsive]")
{
    ArrangementDocument document;
    document.nodes = {
        makeNode(1, {0.5f, 0.5f}, 0.6f, {100.f, 100.f}),
        makeNode(2, {0.5f, 0.5f}, 0.6f, {100.f, 100.f})};

    const LogicalSize viewport{100.f, 100.f};
    const auto layout = computeResponsiveLayout(document, viewport);
    CHECK(layout.sharedScale > 0.f);
    CHECK(layout.sharedScale < 1.f);
    checkValidLayout(layout, viewport);
}

TEST_CASE("GUI Lab normalized layout remains valid across catalog screens",
          "[gui-lab][arrangement][responsive]")
{
    ArrangementDocument document;
    document.nodes = {
        makeNode(1, {0.5f, 0.f}, 0.9f, {230.f, 116.f},
                 {AnchorAxis::centre, AnchorAxis::start}),
        makeNode(2, {0.2f, 0.55f}, 0.28f, {85.f, 84.f}),
        makeNode(3, {0.75f, 0.55f}, 0.24f, {72.f, 80.f})};

    for (const auto &device : getDeviceProfiles())
    {
        for (const auto orientation : {Orientation::portrait,
                                       Orientation::landscape})
        {
            const auto viewport = getEffectiveDeviceSize(device, orientation);
            const auto layout = computeResponsiveLayout(document, viewport);
            CHECK(layout.sharedScale > 0.f);
            checkValidLayout(layout, viewport);
        }
    }
    for (const auto viewport : {LogicalSize{240.f, 1000.f},
                                LogicalSize{1200.f, 240.f}})
    {
        checkValidLayout(computeResponsiveLayout(document, viewport), viewport);
    }
}

TEST_CASE("GUI Lab fixed groups preserve displayed child geometry",
          "[gui-lab][arrangement][responsive][group]")
{
    const LogicalSize viewport{100.f, 200.f};
    auto first = makeNode(1, {0.1f, 0.1f}, 0.2f, {20.f, 10.f},
                          {AnchorAxis::start, AnchorAxis::start});
    first.catalogId = "cursor";
    auto second = makeNode(2, {0.6f, 0.075f}, 0.1f, {10.f, 30.f},
                           {AnchorAxis::start, AnchorAxis::start});
    second.catalogId = "data-wheel";
    ArrangementDocument document{{first, second}};
    const auto originalLayout = computeResponsiveLayout(document, viewport);
    REQUIRE(originalLayout.sharedScale == Catch::Approx(1.f));

    const auto group =
        makeFixedGroup(3, document.nodes, originalLayout, viewport);
    REQUIRE(group.isGroup());
    CHECK(group.referenceSize.width == Catch::Approx(60.f));
    CHECK(group.referenceSize.height == Catch::Approx(30.f));
    REQUIRE(group.children.size() == 2);

    ArrangementDocument grouped{{group}};
    const auto groupLayout = computeResponsiveLayout(grouped, viewport);
    REQUIRE(groupLayout.nodes.size() == 1);
    const auto children = ungroupFixedGroup(
        group, groupLayout.nodes[0].geometry, viewport);
    ArrangementDocument ungrouped{children};
    const auto restoredLayout = computeResponsiveLayout(ungrouped, viewport);
    REQUIRE(restoredLayout.nodes.size() == originalLayout.nodes.size());
    for (const auto &original : originalLayout.nodes)
    {
        const auto *restored =
            findProjectedGeometry(restoredLayout, original.id);
        REQUIRE(restored != nullptr);
        CHECK(restored->position.x ==
              Catch::Approx(original.geometry.position.x));
        CHECK(restored->position.y ==
              Catch::Approx(original.geometry.position.y));
        CHECK(restored->size.width ==
              Catch::Approx(original.geometry.size.width));
        CHECK(restored->size.height ==
              Catch::Approx(original.geometry.size.height));
    }
}

TEST_CASE("GUI Lab grouping a compressed layout bakes valid normalized geometry",
          "[gui-lab][arrangement][responsive][group]")
{
    const LogicalSize viewport{100.f, 100.f};
    auto first = makeNode(1, {0.5f, 0.5f}, 0.8f, {100.f, 100.f});
    first.catalogId = "cursor";
    auto second = makeNode(2, {0.5f, 0.5f}, 0.8f, {100.f, 100.f});
    second.catalogId = "data-wheel";
    ArrangementDocument document{{first, second}};
    const auto compressed = computeResponsiveLayout(document, viewport);
    REQUIRE(compressed.sharedScale > 0.f);
    REQUIRE(compressed.sharedScale < 1.f);
    REQUIRE(compressed.hasValidPlacement);

    const auto group = makeFixedGroup(3, document.nodes, compressed, viewport);
    REQUIRE(group.isGroup());
    CHECK(group.widthFraction > 0.f);
    CHECK(group.widthFraction <= 1.f);

    ArrangementDocument grouped{{group}};
    std::string error;
    const auto restored = deserializeArrangementDocument(
        serializeArrangementDocument(grouped), error);
    INFO(error);
    REQUIRE(restored.has_value());
    REQUIRE(restored->nodes.size() == 1);
    CHECK(restored->nodes[0].widthFraction ==
          Catch::Approx(group.widthFraction));
}

TEST_CASE("GUI Lab placement finds the nearest non-overlapping gap",
          "[gui-lab][arrangement][collision]")
{
    const std::vector<LogicalRect> obstacles{{{40.f, 0.f}, {20.f, 100.f}}};
    const auto nearest = findNearestAvailablePosition(
        {45.f, 30.f}, {20.f, 20.f}, {100.f, 100.f}, obstacles, false);
    REQUIRE(nearest.has_value());
    CHECK(nearest->x == Catch::Approx(60.f));
    CHECK(nearest->y == Catch::Approx(30.f));
    CHECK(isPlacementValid({*nearest, {20.f, 20.f}}, {100.f, 100.f},
                           obstacles));
}

TEST_CASE("GUI Lab corner resizing preserves the selected pivot",
          "[gui-lab][arrangement][resize]")
{
    const ProjectedNodeGeometry start{{10.f, 20.f}, {100.f, 50.f}, 1.f};
    const LogicalSize larger{120.f, 60.f};
    CHECK(positionForResizedNode(start, larger, ResizeCorner::topLeft, false).x
          == Catch::Approx(-10.f));
    CHECK(positionForResizedNode(start, larger, ResizeCorner::topRight, false).y
          == Catch::Approx(10.f));
    CHECK(positionForResizedNode(start, larger, ResizeCorner::bottomLeft,
                                 false)
              .x == Catch::Approx(-10.f));
    const auto centred = positionForResizedNode(
        start, larger, ResizeCorner::topLeft, true);
    CHECK(centred.x == Catch::Approx(0.f));
    CHECK(centred.y == Catch::Approx(15.f));
}

TEST_CASE("GUI Lab version two designs are device agnostic",
          "[gui-lab][arrangement][serialization]")
{
    ArrangementDocument document;
    auto component = makeNode(
        1, {0.8f, 0.25f}, 0.2f, {48.f, 48.f},
        {AnchorAxis::end, AnchorAxis::centre});
    component.catalogId = "cursor";
    document.nodes = {component};

    const auto contents = serializeArrangementDocument(document);
    const auto json = nlohmann::json::parse(contents);
    CHECK(json.at("version") == 2);
    CHECK_FALSE(json.contains("reference"));
    CHECK_FALSE(json.contains("target"));
    CHECK(json.at("nodes")[0].contains("anchorPosition"));
    CHECK(json.at("nodes")[0].contains("widthFraction"));

    std::string error;
    const auto loaded = deserializeArrangementDocument(contents, error);
    REQUIRE(loaded.has_value());
    REQUIRE(loaded->nodes.size() == 1);
    CHECK(loaded->nodes[0].anchor == component.anchor);
    CHECK(loaded->nodes[0].anchorPosition.x ==
          Catch::Approx(component.anchorPosition.x));
    CHECK(loaded->nodes[0].widthFraction ==
          Catch::Approx(component.widthFraction));
}

TEST_CASE("GUI Lab version one designs migrate from their reference canvas",
          "[gui-lab][arrangement][serialization][migration]")
{
    constexpr auto legacy = R"({
      "format": "vmpc2000xl-gui-lab-arrangement",
      "version": 1,
      "reference": {
        "device": "iphone-2g-3g-3gs",
        "orientation": "portrait",
        "size": { "width": 100, "height": 200 }
      },
      "target": {
        "device": "galaxy-s9-plus",
        "orientation": "landscape"
      },
      "nodes": [{
        "id": 1,
        "type": "component",
        "component": "cursor",
        "position": { "x": 10, "y": 20 },
        "scale": 2,
        "referenceSize": { "width": 20, "height": 10 },
        "anchor": { "horizontal": "centre", "vertical": "end" }
      }]
    })";

    std::string error;
    const auto loaded = deserializeArrangementDocument(legacy, error);
    REQUIRE(loaded.has_value());
    REQUIRE(loaded->nodes.size() == 1);
    CHECK(loaded->nodes[0].widthFraction == Catch::Approx(0.4f));
    CHECK(loaded->nodes[0].anchorPosition.x == Catch::Approx(0.3f));
    CHECK(loaded->nodes[0].anchorPosition.y == Catch::Approx(0.2f));
}

TEST_CASE("GUI Lab arrangement loading rejects invalid normalized files",
          "[gui-lab][arrangement][serialization]")
{
    ArrangementDocument document;
    document.nodes = {makeNode(1, {0.5f, 0.5f}, 1.1f, {48.f, 48.f})};
    std::string error;
    CHECK_FALSE(deserializeArrangementDocument(
                    serializeArrangementDocument(document), error)
                    .has_value());
    CHECK(error.find("normalized") != std::string::npos);

    auto valid = makeNode(1, {0.5f, 0.5f}, 0.2f, {48.f, 48.f});
    document.nodes = {valid, valid};
    CHECK_FALSE(deserializeArrangementDocument(
                    serializeArrangementDocument(document), error)
                    .has_value());
    CHECK(error.find("ID") != std::string::npos);

    CHECK_FALSE(deserializeArrangementDocument(
                    R"({"format":"vmpc2000xl-gui-lab-arrangement","version":99})",
                    error)
                    .has_value());
}
