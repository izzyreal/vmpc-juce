#include <catch2/catch_test_macros.hpp>

#include "gui/mobile/MobileFileWorkflowUtil.hpp"

using namespace vmpc_juce::gui::mobile;

TEST_CASE("MPC import extensions are matched case-insensitively",
          "[vmpc][mobile][files]")
{
    CHECK(hasSupportedMpcFileExtension("kick.wav"));
    CHECK(hasSupportedMpcFileExtension("PROGRAM.PGM"));
    CHECK(hasSupportedMpcFileExtension("song.MiD"));
    CHECK_FALSE(hasSupportedMpcFileExtension("project.zip"));
    CHECK_FALSE(hasSupportedMpcFileExtension("sound.wav.txt"));
}

TEST_CASE("Android document names are made unique case-insensitively",
          "[vmpc][mobile][files]")
{
    juce::StringArray existing{"Project", "Project (2)", "PROJECT (3)"};

    CHECK(makeUniqueDocumentName("Other", existing) == "Other");
    CHECK(makeUniqueDocumentName("Project", existing) == "Project (4)");
}
