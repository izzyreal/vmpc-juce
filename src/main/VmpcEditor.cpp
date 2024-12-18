#define ENABLE_GUI_INSPECTOR 0

#include "VmpcEditor.hpp"
#include "VmpcProcessor.hpp"

#include "gui/vector/Constants.hpp"
#include "gui/vector/View.hpp"

#include "VmpcJuceResourceUtil.hpp"

#include "vf_freetype/vf_FreeTypeFaces.h"

#if ENABLE_GUI_INSPECTOR == 1
#include <melatonin_inspector/melatonin_inspector.h>
#endif

using namespace vmpc_juce;
using namespace vmpc_juce::gui::vector;

VmpcEditor::VmpcEditor(VmpcProcessor& vmpcProcessorToUse)
        : AudioProcessorEditor(vmpcProcessorToUse), vmpcProcessor(vmpcProcessorToUse)
{
    auto fontData = VmpcJuceResourceUtil::getResourceData("fonts/NeutralSans-Bold.ttf");
    for (auto &n : juce::Font::findAllTypefaceNames())
        printf("========== before %s\n", n.toRawUTF8());
    FreeTypeFaces::addFaceFromMemory(1.f, 32.f, true, fontData.data(), fontData.size());
    for (auto &n : juce::Font::findAllTypefaceNames())
        printf("========== after %s\n", n.toRawUTF8());
    const juce::String typefaceName = "Neutral Sans";
    const juce::String typefaceStyle = "Bold";
    nimbusSans = juce::Font("Neutral Sans", "Bold", 12.f);
    //nimbusSans = juce::Font("", "", 12.f);

    fontData = VmpcJuceResourceUtil::getResourceData("fonts/mpc2000xl-faceplate-glyphs.ttf");
    mpc2000xlFaceplateGlyphs = juce::Font(juce::Typeface::createSystemTypefaceFor(fontData.data(), fontData.size()));

    const auto getScale = [&] { return (float) getHeight() / (float) initial_height; };

    const auto getNimbusSansScaled = [&, getScale]() -> juce::Font& {
        nimbusSans.setBold(true);
        nimbusSans.setHeight(Constants::BASE_FONT_SIZE * getScale());
#ifdef _WIN32
        nimbusSans.setBold(true);
#endif
        return nimbusSans;
    };

    const auto getMpc2000xlFaceplateGlyphsScaled = [&, getScale]() -> juce::Font& {
        mpc2000xlFaceplateGlyphs.setHeight(Constants::BASE_FONT_SIZE * getScale());
#ifdef _WIN32
        mpc2000xlFaceplateGlyphs.setBold(true);
#endif
        return mpc2000xlFaceplateGlyphs;
    };

    const std::function<void()> resetWindowSize = [&] {
        setSize((int) (initial_width * initial_scale), (int) (initial_height * initial_scale));
    };

    view = new View(vmpcProcessor.mpc, getScale, getNimbusSansScaled, getMpc2000xlFaceplateGlyphsScaled, vmpcProcessor.showAudioSettingsDialog, resetWindowSize);

    setWantsKeyboardFocus(true);
#if JUCE_IOS
    if (juce::JUCEApplication::isStandaloneApp())
    {
        const auto primaryDisplay = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();

        if (primaryDisplay != nullptr)
        {
            const auto area = primaryDisplay->userArea;
            setSize(area.getWidth(), area.getHeight());
        }
        else
        {
            setSize(vmpcProcessor.lastUIWidth, vmpcProcessor.lastUIHeight);
        }
    }
    else
    {
        setSize(vmpcProcessor.lastUIWidth, vmpcProcessor.lastUIHeight);
    }
#else
    setSize(vmpcProcessor.lastUIWidth, vmpcProcessor.lastUIHeight);
    setResizable(true, true);
    getConstrainer()->setFixedAspectRatio(initial_width / (float)initial_height);
    setLookAndFeel(&lookAndFeel);
#endif
    addAndMakeVisible(view);
#if ENABLE_GUI_INSPECTOR == 1
    inspector = new melatonin::Inspector(*this);
    inspector->setVisible(true);
    inspector->toggle(true);
#endif
}

VmpcEditor::~VmpcEditor()
{
    setLookAndFeel(nullptr);
    delete view;
    delete inspector;
}

void VmpcEditor::resized()
{
    view->setBounds(0, 0, getWidth(), getHeight());
}

