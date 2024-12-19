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
    mainFontData = VmpcJuceResourceUtil::getResourceData("fonts/LiberationSans-Bold.ttf");
    FreeTypeFaces::addFaceFromMemory(1.f, 1.f, true, mainFontData.data(), mainFontData.size());
    //mainFont.setTypefaceName("Neutral Sans");
    //mainFont = juce::Font(FreeTypeFaces::createTypefaceForFont(mainFont));

    const auto getScale = [&] { return (float) getHeight() / (float) initial_height; };

    const auto getMainFontScaled = [&, getScale]() -> juce::Font& {
        mainFont = juce::Font();
        mainFont.setTypefaceName("Liberation Sans");
        mainFont.setTypefaceStyle("bold");
        mainFont.setHeight(Constants::BASE_FONT_SIZE * getScale());
        mainFont = juce::Font(FreeTypeFaces::createTypefaceForFont(mainFont));
        mainFont.setHeight(Constants::BASE_FONT_SIZE * getScale());
        printf("Final font height: %f\n", mainFont.getHeight());
#ifdef _WIN32
        mainFont.setBold(true);
#endif
        return mainFont;
    };

    mpc2000xlFaceplateGlyphsFontData = VmpcJuceResourceUtil::getResourceData("fonts/mpc2000xl-faceplate-glyphs.ttf");
    FreeTypeFaces::addFaceFromMemory(1.f, 1.f, true,
            mpc2000xlFaceplateGlyphsFontData.data(), mpc2000xlFaceplateGlyphsFontData.size(), true);
    mpc2000xlFaceplateGlyphsFont.setTypefaceName("MPC2000XL Faceplate-Glyphs");
    mpc2000xlFaceplateGlyphsFont = juce::Font(FreeTypeFaces::createTypefaceForFont(mpc2000xlFaceplateGlyphsFont));

    const auto getMpc2000xlFaceplateGlyphsScaled = [&, getScale]() -> juce::Font& {
        mpc2000xlFaceplateGlyphsFont.setHeight(Constants::BASE_FONT_SIZE * getScale());
#ifdef _WIN32
        mpc2000xlFaceplateGlyphs.setBold(true);
#endif
        return mpc2000xlFaceplateGlyphsFont;
    };

    const std::function<void()> resetWindowSize = [&] {
        setSize((int) (initial_width * initial_scale), (int) (initial_height * initial_scale));
    };

    view = new View(vmpcProcessor.mpc, getScale, getMainFontScaled, getMpc2000xlFaceplateGlyphsScaled, vmpcProcessor.showAudioSettingsDialog, resetWindowSize);

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

