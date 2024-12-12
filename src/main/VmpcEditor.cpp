#define ENABLE_GUI_INSPECTOR 0

#include "VmpcEditor.hpp"
#include "VmpcProcessor.hpp"

#include "gui/vector/Constants.hpp"
#include "gui/vector/View.hpp"

#include "VmpcJuceResourceUtil.hpp"

#if ENABLE_GUI_INSPECTOR == 1
#include <melatonin_inspector/melatonin_inspector.h>
#endif

using namespace vmpc_juce;
using namespace vmpc_juce::gui::vector;

VmpcEditor::VmpcEditor(VmpcProcessor& vmpcProcessorToUse)
        : AudioProcessorEditor(vmpcProcessorToUse), vmpcProcessor(vmpcProcessorToUse)
{
    const auto fontData = VmpcJuceResourceUtil::getResourceData("fonts/mpc2000xl_faceplate_label_font.otf");

    nimbusSans = juce::Font(juce::Typeface::createSystemTypefaceFor(fontData.data(), fontData.size()));

    const auto getScale = [&] { return (float) getHeight() / (float) initial_height; };
    const auto getNimbusSansScaled = [&, getScale]() -> juce::Font& {
        nimbusSans.setHeight(Constants::BASE_FONT_SIZE * getScale());
#ifdef _WIN32
        nimbusSans.setBold(true);
#endif
        return nimbusSans;
    };

    const std::function<void()> resetWindowSize = [&] {
        setSize((int) (initial_width * initial_scale), (int) (initial_height * initial_scale));
    };

    view = new View(vmpcProcessor.mpc, getScale, getNimbusSansScaled, vmpcProcessor.showAudioSettingsDialog, resetWindowSize);

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
            resetWindowSize();
        }
    }
    else
    {
        resetWindowSize();
    }
#else
    resetWindowSize();
    setResizable(true, true);
    getConstrainer()->setFixedAspectRatio(initial_width / initial_height);
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
    delete view;
    delete inspector;
}

void VmpcEditor::resized()
{
    view->setBounds(0, 0, getWidth(), getHeight());
}

