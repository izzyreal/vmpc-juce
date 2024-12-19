#include <chrono>
#include <thread>
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

class MyLaf : public juce::LookAndFeel_V2 {
    public:
        MyLaf()
        {
            fontData = VmpcJuceResourceUtil::getResourceData("fonts/NeutralSans-Bold.ttf");
            FreeTypeFaces::addFaceFromMemory(
                    1.f, 32.f,
                    true,
                    reinterpret_cast<unsigned char*>(fontData.data()),
                    fontData.size());
        }

        juce::Typeface::Ptr getTypefaceForFont(const juce::Font &font) override
        {
            juce::Typeface::Ptr tf;

            juce::String faceName (font.getTypefaceName());

            if (faceName == "Poepie")
            {
                juce::Font f(font);
                f.setTypefaceName ("Neutral Sans");
                tf = FreeTypeFaces::createTypefaceForFont (f);
            }

            if (!tf)
                tf = LookAndFeel::getTypefaceForFont (font);

            return tf;
        }
    private:
        std::vector<char> fontData;

};

VmpcEditor::VmpcEditor(VmpcProcessor& vmpcProcessorToUse)
    : AudioProcessorEditor(vmpcProcessorToUse), vmpcProcessor(vmpcProcessorToUse)
{
    juce::Typeface::clearTypefaceCache();
    globalFontLaf = new MyLaf();
    juce::LookAndFeel::setDefaultLookAndFeel(globalFontLaf);
    auto fontData = VmpcJuceResourceUtil::getResourceData("fonts/SageFalcone.ttf");
    nimbusSans.setTypefaceName("Poepie");
    //nimbusSans = juce::Font(juce::Typeface::createSystemTypefaceFor(fontData.data(), fontData.size()));
    //FreeTypeFaces::addFaceFromMemory(1.f, 32.f, true, fontData.data(), fontData.size());
    //nimbusSans.setTypefaceName("Sage Falcone");
    //nimbusSans.setHeight(14.f);
    //nimbusSansTypeface = FreeTypeFaces::createTypefaceForFont(nimbusSans);
    //nimbusSans2 = juce::Font(nimbusSansTypeface);
    //nimbusSans = juce::Font("", "", 12.f);

    fontData = VmpcJuceResourceUtil::getResourceData("fonts/mpc2000xl-faceplate-glyphs.ttf");
    mpc2000xlFaceplateGlyphs = juce::Font(juce::Typeface::createSystemTypefaceFor(fontData.data(), fontData.size()));

    const auto getScale = [&] { return (float) getHeight() / (float) initial_height; };

    const auto getNimbusSansScaled = [&, getScale]() -> juce::Font& {
        nimbusSans.setHeight(Constants::BASE_FONT_SIZE * getScale());
        //nimbusSans.setTypefaceName("Neutral Sans");
        //nimbusSansTypeface = FreeTypeFaces::createTypefaceForFont(nimbusSans);
        //nimbusSans2 = juce::Font(nimbusSansTypeface);
        //nimbusSans = juce::Font("Sage Falcone", "", Constants::BASE_FONT_SIZE * getScale());
        //nimbusSansTypeface = FreeTypeFaces::createTypefaceForFont(nimbusSans);
        //nimbusSans = juce::Font(nimbusSansTypeface);
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
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    delete globalFontLaf;
    setLookAndFeel(nullptr);
    delete view;
    delete inspector;
}

void VmpcEditor::resized()
{
    view->setBounds(0, 0, getWidth(), getHeight());
}

