#define ENABLE_GUI_INSPECTOR 0

#include "VmpcEditor.hpp"
#include "VmpcProcessor.hpp"

#include "gui/vector/Constants.hpp"
#include "gui/vector/View.hpp"

#include "ResourceUtil.h"

#include <melatonin_inspector/melatonin_inspector.h>

VmpcEditor::VmpcEditor(VmpcProcessor& vmpcProcessorToUse)
        : AudioProcessorEditor(vmpcProcessorToUse), vmpcProcessor(vmpcProcessorToUse)
{
    const auto fontData = mpc::ResourceUtil::get_resource_data("fonts/mpc2000xl_faceplate_label_font.otf");

    nimbusSans = juce::Font(juce::Typeface::createSystemTypefaceFor(fontData.data(), fontData.size()));

    const auto getScale = [&] { return (float) getHeight() / (float) initial_height; };
    const auto getNimbusSansScaled = [&, getScale]() -> juce::Font& {
        nimbusSans.setHeight(Constants::BASE_FONT_SIZE * getScale());
        return nimbusSans;
    };

    view = new View(vmpcProcessor.mpc, getScale, getNimbusSansScaled);

    setSize((int) (initial_width * initial_scale), (int) (initial_height * initial_scale));
    setWantsKeyboardFocus(true);

    setResizable(true, true);
    getConstrainer()->setFixedAspectRatio(initial_width / initial_height);
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

