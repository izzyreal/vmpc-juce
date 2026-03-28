#include <catch2/catch_test_macros.hpp>

#include "ZipSaveTarget.hpp"
#include "gui/vector/Node.hpp"

#include <nlohmann/json.hpp>

using namespace vmpc_juce;
using namespace vmpc_juce::gui::vector;

TEST_CASE("ZipSaveTarget round-trips stored files", "[vmpc][zip-save-target]")
{
    ZipSaveTarget target;

    REQUIRE(target.setFileData("screen.txt", {'m', 'i', 'x', 'e', 'r'}));
    REQUIRE(target.setFileData("selectedPad.txt", {42}));

    const auto zipBlock = target.toZipMemoryBlock();
    REQUIRE(zipBlock);
    REQUIRE(zipBlock->getSize() > 0);

    ZipSaveTarget restored(zipBlock->getData(), zipBlock->getSize());

    const auto screenRes = restored.getFileData("screen.txt");
    REQUIRE(screenRes);
    CHECK(std::string(screenRes->begin(), screenRes->end()) == "mixer");

    const auto selectedPadRes = restored.getFileData("selectedPad.txt");
    REQUIRE(selectedPadRes);
    REQUIRE(selectedPadRes->size() == 1);
    CHECK(static_cast<unsigned char>((*selectedPadRes)[0]) == 42);

    const auto existsRes = restored.exists("screen.txt");
    REQUIRE(existsRes);
    CHECK(*existsRes);
}

TEST_CASE("ZipSaveTarget removes files on empty writes", "[vmpc][zip-save-target]")
{
    ZipSaveTarget target;

    REQUIRE(target.setFileData("focus.txt", {'s', 'q'}));
    REQUIRE(target.setFileData("focus.txt", {}));

    const auto existsRes = target.exists("focus.txt");
    REQUIRE(existsRes);
    CHECK_FALSE(*existsRes);

    const auto readRes = target.getFileData("focus.txt");
    CHECK_FALSE(readRes.has_value());
}

TEST_CASE("ZipSaveTarget tolerates invalid zip input", "[vmpc][zip-save-target]")
{
    const std::vector<char> garbage{'n', 'o', 't', '-', 'a', '-', 'z', 'i', 'p'};

    REQUIRE_NOTHROW(ZipSaveTarget(garbage.data(), garbage.size()));

    ZipSaveTarget restored(garbage.data(), garbage.size());
    const auto existsRes = restored.exists("anything");
    REQUIRE(existsRes);
    CHECK_FALSE(*existsRes);
}

TEST_CASE("Vector node parsing keeps safe defaults for sparse layouts",
          "[vmpc][vector-layout]")
{
    const auto data = nlohmann::json::parse(R"json(
        {
            "type": "flex_box",
            "children": [
                { "type": "grid", "area": [0, 0, 0, 0] }
            ]
        }
    )json");

    const auto parsed = data.get<node>();

    CHECK(parsed.node_type == "flex_box");
    CHECK(parsed.base_width == 0);
    CHECK(parsed.base_height == 0);
    CHECK(parsed.margin == 0.0f);
    CHECK(parsed.shadow_size == 0.0f);
    REQUIRE(parsed.children.size() == 1);
    REQUIRE(parsed.children[0].area.size() == 4);
    CHECK(parsed.children[0].area[2] == 1);
    CHECK(parsed.children[0].area[3] == 1);
}
