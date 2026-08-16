#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "gui/arrangement/ArrangementCatalog.hpp"
#include "gui/arrangement/ArrangementModel.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <string>

using namespace vmpc_juce::gui::arrangement;

namespace
{
    std::string arrangementId(const char digit)
    {
        return std::string(31, '0') + digit;
    }

    ArrangementNodeModel makeNode(const std::uint64_t id,
                                  const LogicalPoint center,
                                  const float widthFraction,
                                  const LogicalSize referenceSize)
    {
        ArrangementNodeModel node;
        node.id = id;
        node.catalogId = "cursor";
        node.center = center;
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
                CHECK_FALSE(rectanglesOverlap({first.position, first.size},
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
    CHECK(compactMountedLcdReferenceSize.height == Catch::Approx(88.19710f));
}

TEST_CASE("GUI Lab catalog contains compact function and vertical main assets",
          "[gui-lab][arrangement][catalog]")
{
    const auto *functionKeys = findCatalogEntry("function-buttons-compact");
    REQUIRE(functionKeys != nullptr);
    CHECK(std::string(functionKeys->resourceName) ==
          "components/f_keys_unlabelled_trimmed");
    CHECK(functionKeys->referenceWidth == Catch::Approx(180.f));
    CHECK(functionKeys->referenceHeight == Catch::Approx(10.f));

    const auto *mainOpen = findCatalogEntry("main-open-vertical");
    REQUIRE(mainOpen != nullptr);
    CHECK(std::string(mainOpen->resourceName) ==
          "components/main_screen_and_open_window_vertical");
    CHECK(mainOpen->referenceWidth == Catch::Approx(45.f));
    CHECK(mainOpen->referenceHeight == Catch::Approx(42.f));
}

TEST_CASE("GUI Lab catalog contains trimmed knob variants",
          "[gui-lab][arrangement][catalog]")
{
    const auto *vertical = findCatalogEntry("volume-gain-vertical");
    REQUIRE(vertical != nullptr);
    CHECK(std::string(vertical->resourceName) ==
          "components/main_volume_rec_gain_vertical_trimmed");
    CHECK(vertical->referenceWidth == Catch::Approx(42.f));
    CHECK(vertical->referenceHeight == Catch::Approx(76.f));

    const auto *mainVolume = findCatalogEntry("main-volume");
    REQUIRE(mainVolume != nullptr);
    CHECK(std::string(mainVolume->resourceName) ==
          "components/main_volume_trimmed");
    CHECK(mainVolume->referenceWidth == Catch::Approx(42.f));
    CHECK(mainVolume->referenceHeight == Catch::Approx(38.f));

    const auto *recGain = findCatalogEntry("rec-gain");
    REQUIRE(recGain != nullptr);
    CHECK(std::string(recGain->resourceName) == "components/rec_gain_trimmed");
    CHECK(recGain->referenceWidth == Catch::Approx(42.f));
    CHECK(recGain->referenceHeight == Catch::Approx(38.f));
}

TEST_CASE("GUI Lab catalog contains isolated logo assets",
          "[gui-lab][arrangement][catalog]")
{
    const auto *akaiLogo = findCatalogEntry("akai-logo");
    REQUIRE(akaiLogo != nullptr);
    CHECK(std::string(akaiLogo->resourceName) == "components/akai_logo");
    CHECK(akaiLogo->referenceWidth == Catch::Approx(53.793735f));
    CHECK(akaiLogo->referenceHeight == Catch::Approx(22.965294f));

    const auto *mpcLogo = findCatalogEntry("mpc2000xl-logo");
    REQUIRE(mpcLogo != nullptr);
    CHECK(std::string(mpcLogo->resourceName) ==
          "components/mpc2000xl_logo");
    CHECK(mpcLogo->referenceWidth == Catch::Approx(112.635391f));
    CHECK(mpcLogo->referenceHeight == Catch::Approx(20.439058f));
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
    CHECK(constrainItemScale(3.f, reference, {390.f, 844.f}) == 390.f / 230.f);
}

TEST_CASE("GUI Lab normalized centre geometry projects onto a target",
          "[gui-lab][arrangement][responsive]")
{
    const LogicalSize viewport{400.f, 300.f};
    const auto node = makeNode(1, {0.25f, 0.5f}, 0.5f, {100.f, 50.f});
    const auto projected = projectNode(node, viewport);
    CHECK(projected.size.width == Catch::Approx(200.f));
    CHECK(projected.size.height == Catch::Approx(100.f));
    CHECK(projected.scale == Catch::Approx(2.f));
    CHECK(projected.position.x == Catch::Approx(0.f));
    CHECK(projected.position.y == Catch::Approx(100.f));

    const auto center = normalizedCenter(projected, viewport);
    CHECK(center.x == Catch::Approx(node.center.x));
    CHECK(center.y == Catch::Approx(node.center.y));
}

TEST_CASE("GUI Lab responsive scaling preserves component centres",
          "[gui-lab][arrangement][responsive]")
{
    const LogicalSize viewport{400.f, 300.f};
    const auto node = makeNode(1, {0.5f, 0.4f}, 0.5f, {100.f, 50.f});
    const auto full = projectNodeAtScale(node, viewport, 1.f);
    const auto compressed = projectNodeAtScale(node, viewport, 0.5f);

    CHECK(full.position.x == Catch::Approx(100.f));
    CHECK(full.position.y == Catch::Approx(70.f));
    CHECK(full.size.width == Catch::Approx(200.f));
    CHECK(full.size.height == Catch::Approx(100.f));
    CHECK(compressed.position.x == Catch::Approx(150.f));
    CHECK(compressed.position.y == Catch::Approx(95.f));
    CHECK(compressed.size.width == Catch::Approx(100.f));
    CHECK(compressed.size.height == Catch::Approx(50.f));
    CHECK(compressed.position.x + compressed.size.width * 0.5f ==
          Catch::Approx(full.position.x + full.size.width * 0.5f));
    CHECK(compressed.position.y + compressed.size.height * 0.5f ==
          Catch::Approx(full.position.y + full.size.height * 0.5f));
}

TEST_CASE("GUI Lab responsive solver centres compressed edge components",
          "[gui-lab][arrangement][responsive]")
{
    ArrangementDocument document;
    document.nodes = {makeNode(1, {0.5f, 0.5f}, 1.f, {100.f, 200.f})};

    const LogicalSize viewport{100.f, 100.f};
    const auto layout = computeResponsiveLayout(document, viewport);
    REQUIRE(layout.nodes.size() == 1);
    CHECK(layout.sharedScale == Catch::Approx(0.5f).margin(0.0001f));

    const auto &geometry = layout.nodes.front().geometry;
    CHECK(geometry.position.x + geometry.size.width * 0.5f ==
          Catch::Approx(viewport.width * 0.5f));
    CHECK(geometry.position.x > 0.f);
    CHECK(geometry.position.y == Catch::Approx(0.f).margin(0.0011f));
}

TEST_CASE("GUI Lab centre-scaled projection has an editing inverse",
          "[gui-lab][arrangement][responsive]")
{
    const LogicalSize viewport{400.f, 300.f};
    const auto node = makeNode(1, {0.3f, 0.4f}, 0.5f, {100.f, 50.f});
    auto displayed = projectNodeAtScale(node, viewport, 0.5f);

    const auto originalCenter = normalizedCenter(displayed, viewport);
    CHECK(originalCenter.x == Catch::Approx(node.center.x));
    CHECK(originalCenter.y == Catch::Approx(node.center.y));

    displayed.position.x += 20.f;
    displayed.position.y += 15.f;
    const auto movedCenter = normalizedCenter(displayed, viewport);
    auto moved = node;
    moved.center = movedCenter;
    const auto restored = projectNodeAtScale(moved, viewport, 0.5f);
    CHECK(restored.position.x == Catch::Approx(displayed.position.x));
    CHECK(restored.position.y == Catch::Approx(displayed.position.y));
    CHECK(restored.size.width == Catch::Approx(displayed.size.width));
    CHECK(restored.size.height == Catch::Approx(displayed.size.height));
}

TEST_CASE("GUI Lab adapted components can be moved to the screen edge",
          "[gui-lab][arrangement][responsive]")
{
    const LogicalSize viewport{320.f, 480.f};
    auto node = makeNode(1, {0.5f, 0.087f}, 1.f, {222.52338f, 84.19710f});
    auto displayed = projectNodeAtScale(node, viewport, 0.69f);
    displayed.position.y = 0.f;

    node.center = normalizedCenter(displayed, viewport);
    const auto restored = projectNodeAtScale(node, viewport, 0.69f);
    CHECK(restored.position.y == Catch::Approx(0.f).margin(0.0001f));
    CHECK(node.center.y > 0.f);
}

TEST_CASE("GUI Lab width fraction one spans the available width",
          "[gui-lab][arrangement][responsive]")
{
    ArrangementDocument document;
    document.nodes = {makeNode(1, {0.5f, 0.5f}, 1.f, {200.f, 50.f})};

    for (const auto viewport :
         {LogicalSize{320.f, 568.f}, LogicalSize{428.f, 926.f},
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

TEST_CASE("GUI Lab normalized geometry adapts without mutating the document",
          "[gui-lab][arrangement][responsive]")
{
    ArrangementDocument document{
        {makeNode(1, {0.3f, 0.4f}, 0.4f, {100.f, 50.f})}};

    for (const auto viewport :
         {LogicalSize{320.f, 568.f}, LogicalSize{844.f, 390.f}})
    {
        const auto layout = computeResponsiveLayout(document, viewport);
        REQUIRE(layout.nodes.size() == 1);
        REQUIRE(layout.sharedScale == Catch::Approx(1.f));
        const auto &geometry = layout.nodes.front().geometry;
        CHECK(geometry.position.x + geometry.size.width * 0.5f ==
              Catch::Approx(viewport.width * 0.3f));
        CHECK(geometry.position.y + geometry.size.height * 0.5f ==
              Catch::Approx(viewport.height * 0.4f));
        CHECK(geometry.size.width == Catch::Approx(viewport.width * 0.4f));
        CHECK(geometry.size.height == Catch::Approx(viewport.width * 0.2f));
    }

    CHECK(document.nodes.front().center.x == Catch::Approx(0.3f));
    CHECK(document.nodes.front().center.y == Catch::Approx(0.4f));
    CHECK(document.nodes.front().widthFraction == Catch::Approx(0.4f));
}

TEST_CASE("GUI Lab responsive layout compresses without moving centres",
          "[gui-lab][arrangement][responsive]")
{
    ArrangementDocument document;
    document.nodes = {makeNode(1, {0.35f, 0.5f}, 0.6f, {100.f, 100.f}),
                      makeNode(2, {0.65f, 0.5f}, 0.6f, {100.f, 100.f})};

    const LogicalSize viewport{100.f, 100.f};
    const auto layout = computeResponsiveLayout(document, viewport);
    CHECK(layout.sharedScale > 0.f);
    CHECK(layout.sharedScale < 1.f);
    checkValidLayout(layout, viewport);
    for (size_t i = 0; i < layout.nodes.size(); ++i)
    {
        const auto center =
            normalizedCenter(layout.nodes[i].geometry, viewport);
        CHECK(center.x == Catch::Approx(document.nodes[i].center.x));
        CHECK(center.y == Catch::Approx(document.nodes[i].center.y));
    }
}

TEST_CASE("GUI Lab normalized layout remains valid across catalog screens",
          "[gui-lab][arrangement][responsive]")
{
    ArrangementDocument document;
    document.nodes = {makeNode(1, {0.5f, 0.15f}, 0.9f, {230.f, 116.f}),
                      makeNode(2, {0.3f, 0.65f}, 0.28f, {85.f, 84.f}),
                      makeNode(3, {0.75f, 0.65f}, 0.24f, {72.f, 80.f})};

    for (const auto &device : getDeviceProfiles())
    {
        for (const auto orientation :
             {Orientation::portrait, Orientation::landscape})
        {
            const auto viewport = getEffectiveDeviceSize(device, orientation);
            const auto layout = computeResponsiveLayout(document, viewport);
            CHECK(layout.sharedScale > 0.f);
            checkValidLayout(layout, viewport);
        }
    }
    for (const auto viewport :
         {LogicalSize{240.f, 1000.f}, LogicalSize{1200.f, 240.f}})
    {
        checkValidLayout(computeResponsiveLayout(document, viewport), viewport);
    }
}

TEST_CASE("GUI Lab tall iPhone layout preserves its structure on iPhone 2G",
          "[gui-lab][arrangement][responsive]")
{
    ArrangementDocument document;
    document.nodes = {makeNode(1, {0.5f, 0.087f}, 1.f, {222.52338f, 84.19710f}),
                      makeNode(2, {0.5f, 0.207f}, 1.f, {180.f, 25.f}),
                      makeNode(3, {0.5f, 0.434f}, 0.8426667f, {85.f, 84.f}),
                      makeNode(4, {0.786f, 0.737f}, 0.4128024f, {72.f, 80.f}),
                      makeNode(5, {0.320f, 0.657f}, 0.4906667f, {90.f, 21.f}),
                      makeNode(6, {0.309f, 0.759f}, 0.5001716f, {48.f, 31.f}),
                      makeNode(7, {0.5f, 0.879f}, 1.f, {179.f, 28.f}),
                      makeNode(8, {0.5f, 0.961f}, 1.f, {179.f, 30.f})};

    const LogicalSize iphone2G{320.f, 480.f};
    const auto layout = computeResponsiveLayout(document, iphone2G);
    CHECK(layout.sharedScale > 0.65f);
    checkValidLayout(layout, iphone2G);

    for (size_t i = 0; i < layout.nodes.size(); ++i)
    {
        const auto center =
            normalizedCenter(layout.nodes[i].geometry, iphone2G);
        CHECK(center.x == Catch::Approx(document.nodes[i].center.x));
        CHECK(center.y == Catch::Approx(document.nodes[i].center.y));
    }

    const auto *lcd = findProjectedGeometry(layout, 1);
    const auto *dataWheel = findProjectedGeometry(layout, 4);
    const auto *cursor = findProjectedGeometry(layout, 6);
    const auto *locate = findProjectedGeometry(layout, 7);
    const auto *transport = findProjectedGeometry(layout, 8);
    REQUIRE(lcd != nullptr);
    REQUIRE(dataWheel != nullptr);
    REQUIRE(cursor != nullptr);
    REQUIRE(locate != nullptr);
    REQUIRE(transport != nullptr);
    CHECK(lcd->position.y < 1.f);
    CHECK(locate->position.y >=
          Catch::Approx(dataWheel->position.y + dataWheel->size.height)
              .margin(0.01f));
    CHECK(
        locate->position.y >=
        Catch::Approx(cursor->position.y + cursor->size.height).margin(0.01f));
    CHECK(
        transport->position.y >=
        Catch::Approx(locate->position.y + locate->size.height).margin(0.01f));
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
    CHECK(
        isPlacementValid({*nearest, {20.f, 20.f}}, {100.f, 100.f}, obstacles));
}

TEST_CASE("GUI Lab snapped keyboard movement skips blocked grid positions",
          "[gui-lab][arrangement][collision]")
{
    const LogicalSize bounds{100.f, 100.f};
    const std::vector<LogicalRect> moving{{{10.f, 40.f}, {10.f, 10.f}}};
    const std::vector<LogicalRect> obstacles{{{22.f, 0.f}, {4.f, 100.f}}};

    const auto snapped = findNearestAvailableAxisTranslation(
        {4.f, 0.f}, moving, bounds, obstacles, 4.f);
    REQUIRE(snapped.has_value());
    CHECK(snapped->x == Catch::Approx(16.f));
    CHECK(snapped->y == Catch::Approx(0.f));

    CHECK_FALSE(findNearestAvailableAxisTranslation({4.f, 0.f}, moving, bounds,
                                                    obstacles)
                    .has_value());
}

TEST_CASE("GUI Lab keyboard movement advances from a nearly aligned grid edge",
          "[gui-lab][arrangement][collision][regression]")
{
    auto mainOpen = makeNode(9, {0.5846154093742371f, 0.7687203884124756f},
                             0.24615386128425598f, {45.f, 42.f});
    mainOpen.catalogId = "main-open-vertical";
    const LogicalSize bounds{390.f, 844.f};
    const auto geometry = projectNode(mainOpen, bounds);
    REQUIRE(geometry.position.x > 180.f);
    REQUIRE(geometry.position.x < 180.001f);

    const auto requestedTranslation =
        snapAxisTranslationToGrid(geometry.position.x, -4.f, 4.f);
    CHECK(requestedTranslation == Catch::Approx(-4.f).margin(0.0001f));

    const std::vector<LogicalRect> moving{{geometry.position, geometry.size}};
    const std::vector<LogicalRect> obstacles{
        {{0.f, 600.f}, {144.f, 93.f}},
        {{276.0688f, 589.4108f}, {113.9312f, 126.5902f}}};
    const auto translation = findNearestAvailableAxisTranslation(
        {requestedTranslation, 0.f}, moving, bounds, obstacles, 4.f);
    REQUIRE(translation.has_value());
    CHECK(geometry.position.x + translation->x ==
          Catch::Approx(176.f).margin(0.0001f));
}

TEST_CASE("GUI Lab directional grid snapping tolerates projection residue",
          "[gui-lab][arrangement][collision]")
{
    CHECK(snapAxisTranslationToGrid(180.00002f, -4.f) ==
          Catch::Approx(-4.00002f));
    CHECK(snapAxisTranslationToGrid(179.99998f, 4.f) ==
          Catch::Approx(4.00002f));
    CHECK(snapAxisTranslationToGrid(181.f, -4.f) == Catch::Approx(-1.f));
    CHECK(snapAxisTranslationToGrid(181.f, 4.f) == Catch::Approx(3.f));
    CHECK(snapAxisTranslationToGrid(180.00002f, -16.f) ==
          Catch::Approx(-16.00002f));
}

TEST_CASE("GUI Lab keyboard movement clamps its final step to the edge",
          "[gui-lab][arrangement][collision]")
{
    const std::vector<LogicalRect> moving{{{89.f, 40.f}, {10.f, 10.f}}};
    const auto translation = findNearestAvailableAxisTranslation(
        {4.f, 0.f}, moving, {100.f, 100.f}, {}, 4.f);
    REQUIRE(translation.has_value());
    CHECK(translation->x == Catch::Approx(1.f));
    CHECK(translation->y == Catch::Approx(0.f));
}

TEST_CASE("GUI Lab corner resizing preserves the selected pivot",
          "[gui-lab][arrangement][resize]")
{
    const ProjectedNodeGeometry start{{10.f, 20.f}, {100.f, 50.f}, 1.f};
    const LogicalSize larger{120.f, 60.f};
    CHECK(
        positionForResizedNode(start, larger, ResizeCorner::topLeft, false).x ==
        Catch::Approx(-10.f));
    CHECK(positionForResizedNode(start, larger, ResizeCorner::topRight, false)
              .y == Catch::Approx(10.f));
    CHECK(positionForResizedNode(start, larger, ResizeCorner::bottomLeft, false)
              .x == Catch::Approx(-10.f));
    const auto centred =
        positionForResizedNode(start, larger, ResizeCorner::topLeft, true);
    CHECK(centred.x == Catch::Approx(0.f));
    CHECK(centred.y == Catch::Approx(15.f));
}

TEST_CASE("GUI Lab version four designs use normalized centre geometry",
          "[gui-lab][arrangement][serialization]")
{
    ArrangementDocument document;
    auto component = makeNode(1, {0.8f, 0.25f}, 0.2f, {48.f, 48.f});
    component.catalogId = "cursor";
    document.nodes = {component};

    const auto contents = serializeArrangementDocument(document);
    const auto json = nlohmann::json::parse(contents);
    CHECK(json.at("version") == 4);
    CHECK_FALSE(json.contains("reference"));
    CHECK_FALSE(json.contains("target"));
    CHECK(json.at("nodes")[0].contains("center"));
    CHECK_FALSE(json.at("nodes")[0].contains("position"));
    CHECK(json.at("nodes")[0].contains("widthFraction"));
    CHECK_FALSE(json.at("nodes")[0].contains("anchor"));
    CHECK_FALSE(json.at("nodes")[0].contains("anchorPosition"));
    CHECK_FALSE(json.at("nodes")[0].contains("type"));
    CHECK_FALSE(json.at("nodes")[0].contains("children"));

    std::string error;
    const auto loaded = deserializeArrangementDocument(contents, error);
    REQUIRE(loaded.has_value());
    REQUIRE(loaded->nodes.size() == 1);
    CHECK(loaded->nodes[0].center.x == Catch::Approx(component.center.x));
    CHECK(loaded->nodes[0].center.y == Catch::Approx(component.center.y));
    CHECK(loaded->nodes[0].widthFraction ==
          Catch::Approx(component.widthFraction));
}

TEST_CASE("GUI Lab old arrangement versions are unsupported",
          "[gui-lab][arrangement][serialization]")
{
    constexpr auto oldVersion = R"({
      "format": "vmpc2000xl-arrangement",
      "version": 3,
      "nodes": []
    })";

    std::string error;
    CHECK_FALSE(deserializeArrangementDocument(oldVersion, error).has_value());
    CHECK(error.find("unsupported design version") != std::string::npos);
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

    CHECK_FALSE(
        deserializeArrangementDocument(
            R"({"format":"vmpc2000xl-gui-lab-arrangement","version":99})",
            error)
            .has_value());
}

TEST_CASE("Arrangement setup round-trips five ordered slots",
          "[arrangement][setup]")
{
    ArrangementSetup setup;
    ArrangementSlot first;
    first.id = arrangementId('1');
    first.orientation = Orientation::portrait;
    first.arrangement.nodes = {makeNode(1, {0.5f, 0.1f}, 1.f, {210.f, 55.f})};
    first.arrangement.nodes.front().catalogId = "lcd-bare";
    setup.slots[0] = first;

    ArrangementSlot fifth;
    fifth.id = arrangementId('2');
    fifth.orientation = Orientation::landscape;
    fifth.arrangement.nodes = {makeNode(2, {0.5f, 0.5f}, 0.25f, {48.f, 31.f})};
    fifth.arrangement.nodes.front().catalogId = "cursor-compact";
    setup.slots[4] = fifth;

    const auto contents = serializeArrangementSetup(setup);
    const auto json = nlohmann::json::parse(contents);
    CHECK(json.at("format") == "vmpc2000xl-arrangement-setup");
    CHECK(json.at("version") == 4);
    CHECK(json.at("slots").size() == 5);
    CHECK(json.at("slots")[1].is_null());
    CHECK(json.at("slots")[0].at("id") == arrangementId('1'));
    CHECK(json.at("slots")[4].at("orientation") == "landscape");

    std::string error;
    const auto restored = deserializeArrangementSetup(contents, error);
    INFO(error);
    REQUIRE(restored.has_value());
    REQUIRE(restored->slots[0].has_value());
    CHECK(restored->slots[0]->id == arrangementId('1'));
    CHECK(restored->slots[0]->orientation == Orientation::portrait);
    REQUIRE(restored->slots[4].has_value());
    CHECK(restored->slots[4]->orientation == Orientation::landscape);
    CHECK(findFirstOccupiedSlot(*restored) == std::optional<std::size_t>(0));
}

TEST_CASE("Arrangement setup rejects invalid shape and catalog entries",
          "[arrangement][setup]")
{
    std::string error;
    CHECK_FALSE(
        deserializeArrangementSetup(
            R"({"format":"vmpc2000xl-arrangement-setup","version":3,"slots":[null,null,null,null,null]})",
            error)
            .has_value());
    CHECK(error.find("unsupported setup version") != std::string::npos);

    CHECK_FALSE(
        deserializeArrangementSetup(
            R"({"format":"vmpc2000xl-arrangement-setup","version":4,"slots":[]})",
            error)
            .has_value());
    CHECK(error.find("exactly five") != std::string::npos);

    ArrangementSetup setup;
    ArrangementSlot slot;
    slot.id = arrangementId('1');
    slot.arrangement.nodes = {makeNode(1, {0.5f, 0.5f}, 0.2f, {48.f, 48.f})};
    slot.arrangement.nodes.front().catalogId = "not-in-the-catalog";
    setup.slots[2] = slot;
    CHECK_FALSE(
        deserializeArrangementSetup(serializeArrangementSetup(setup), error)
            .has_value());
    CHECK(error.find("Unknown arrangement component") != std::string::npos);
}

TEST_CASE("Arrangement setup IDs remain stable across reordering",
          "[arrangement][setup]")
{
    ArrangementSetup setup;
    ArrangementSlot first;
    first.id = arrangementId('1');
    first.arrangement.nodes = {makeNode(1, {0.5f, 0.5f}, 0.2f, {48.f, 48.f})};
    setup.slots[0] = first;

    ArrangementSlot second;
    second.id = arrangementId('2');
    second.arrangement.nodes = {makeNode(2, {0.5f, 0.5f}, 0.2f, {48.f, 48.f})};
    setup.slots[1] = second;

    CHECK(resolveArrangementSlot(setup, second.id) ==
          std::optional<std::size_t>(1));
    std::swap(setup.slots[0], setup.slots[1]);
    CHECK(resolveArrangementSlot(setup, second.id) ==
          std::optional<std::size_t>(0));
    CHECK(resolveArrangementSlot(setup, arrangementId('3')) ==
          std::optional<std::size_t>(0));
}

TEST_CASE("Arrangement setup rejects invalid or duplicate IDs",
          "[arrangement][setup]")
{
    ArrangementSetup setup;
    ArrangementSlot slot;
    slot.id = arrangementId('1');
    slot.arrangement.nodes = {makeNode(1, {0.5f, 0.5f}, 0.2f, {48.f, 48.f})};
    setup.slots[0] = slot;
    slot.arrangement.nodes.front().id = 2;
    setup.slots[1] = slot;

    CHECK_THROWS(serializeArrangementSetup(setup));

    auto json = nlohmann::json::parse(
        R"({"format":"vmpc2000xl-arrangement-setup","version":4,"slots":[null,null,null,null,null]})");
    json["slots"][0] = {
        {"id", "not-a-uuid"},
        {"orientation", "portrait"},
        {"arrangement", nlohmann::json::parse(
                            serializeArrangementDocument(slot.arrangement))}};
    std::string error;
    CHECK_FALSE(deserializeArrangementSetup(json.dump(), error).has_value());
    CHECK(error.find("UUID") != std::string::npos);
}

TEST_CASE("Arrangement document reader rejects the legacy format identifier",
          "[arrangement][serialization]")
{
    ArrangementDocument document;
    document.nodes = {makeNode(1, {0.5f, 0.5f}, 0.2f, {48.f, 48.f})};
    auto json = nlohmann::json::parse(serializeArrangementDocument(document));
    CHECK(json.at("format") == "vmpc2000xl-arrangement");
    json["format"] = "vmpc2000xl-gui-lab-arrangement";
    std::string error;
    CHECK_FALSE(deserializeArrangementDocument(json.dump(), error).has_value());
    CHECK(error.find("not a VMPC2000XL arrangement") != std::string::npos);
}
