/*
    This file is part of vmpc-juce, a JUCE implementation of VMPC2000XL.

    vmpc-juce is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License (GPL) as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    vmpc-juce is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with vmpc-juce. If not, see <https://www.gnu.org/licenses/>.

    This project uses JUCE, which is licensed under the GNU Affero General Public License (AGPL).
    See <https://juce.com> for details.
*/
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
    auto fontData = VmpcJuceResourceUtil::getResourceData("fonts/NeutralSans-Bold.ttf");
    auto typeface = juce::Typeface::createSystemTypefaceFor(fontData.data(), fontData.size());
    nimbusSans = new juce::Font(juce::FontOptions(typeface));

    fontData = VmpcJuceResourceUtil::getResourceData("fonts/mpc2000xl-faceplate-glyphs.ttf");
    typeface = juce::Typeface::createSystemTypefaceFor(fontData.data(), fontData.size());
    mpc2000xlFaceplateGlyphs = new juce::Font(juce::FontOptions(typeface));

    const auto getScale = [&] { return (float) getHeight() / (float) initial_height; };

    const auto getNimbusSansScaled = [&, getScale]() -> juce::Font& {
        nimbusSans->setHeight(Constants::BASE_FONT_SIZE * getScale());
#ifdef _WIN32
        nimbusSans->setBold(true);
#endif
        return *nimbusSans;
    };

    const auto getMpc2000xlFaceplateGlyphsScaled = [&, getScale]() -> juce::Font& {
        mpc2000xlFaceplateGlyphs->setHeight(Constants::BASE_FONT_SIZE * getScale());
#ifdef _WIN32
        mpc2000xlFaceplateGlyphs->setBold(true);
#endif
        return *mpc2000xlFaceplateGlyphs;
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
    delete nimbusSans;
    delete mpc2000xlFaceplateGlyphs;
#if ENABLE_GUI_INSPECTOR == 1
    delete inspector;
#endif
}

void VmpcEditor::resized()
{
    view->setBounds(0, 0, getWidth(), getHeight());
}

