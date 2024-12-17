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
#include "VmpcTooltipComponent.hpp"

#include "controls/Controls.hpp"
#include "controls/KbMapping.hpp"

#include <algorithm>

VmpcTooltipComponent::VmpcTooltipComponent(
        mpc::Mpc &_mpc,
        std::shared_ptr<mpc::hardware::HwComponent> _mpcHardwareComponent)
        : Button(_mpcHardwareComponent->getLabel()),
        mpc(_mpc), mpcHardwareComponent(_mpcHardwareComponent)
{
    auto label = mpcHardwareComponent->getLabel();
    auto kbMapping = mpc.getControls()->getKbMapping().lock();
    keyboardMappingText = kbMapping->getKeyCodeString(kbMapping->getKeyCodeFromLabel(label));

    if (std::dynamic_pointer_cast<DummyDataWheelHwComponent>(mpcHardwareComponent))
    {
        keyboardMappingText =
                kbMapping->getKeyCodeString(kbMapping->getKeyCodeFromLabel("datawheel-down")) + "/" +
                kbMapping->getKeyCodeString(kbMapping->getKeyCodeFromLabel("datawheel-up"));
    }

    std::transform(keyboardMappingText.begin(), keyboardMappingText.end(), keyboardMappingText.begin(), ::toupper);

    keyboardMappingText = "Key: " + keyboardMappingText;

    setWantsKeyboardFocus(false);
}

void VmpcTooltipComponent::paintButton(juce::Graphics &g, bool, bool)
{
    if (keyboardMappingOpacity == 0.f)
    {
        return;
    }

    g.setColour(juce::Colours::darkkhaki.darker(0.8f).withAlpha(0.8f * keyboardMappingOpacity));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 5);

    g.setColour(juce::Colours::lightcoral.brighter());
    g.setOpacity(keyboardMappingOpacity);
    g.setFont(g.getCurrentFont().boldened());
    g.drawText(keyboardMappingText.substr(5), getLocalBounds().expanded(30), juce::Justification::centred, false);
}

void VmpcTooltipComponent::mouseMove(const juce::MouseEvent &event)
{
    if (event.mods.isAnyModifierKeyDown() && !event.mods.isShiftDown() && getTooltip() == "")
    {
        setTooltip(keyboardMappingText);
    }
}

void VmpcTooltipComponent::mouseExit(const juce::MouseEvent &)
{
    setTooltip("");

    if (keyboardMappingOpacity > 0.f)
    {
        hideKeyboardMapping();
    }
}

void VmpcTooltipComponent::showKeyboardMapping()
{
    startTimer(20);
    keyboardMappingOpacity = 0.01f;
    keyboardMappingOpacityIncrement = 0.01f;
}

void VmpcTooltipComponent::hideKeyboardMapping()
{
    stopTimer();
    keyboardMappingOpacity = 0.f;
    repaint();
}

void VmpcTooltipComponent::timerCallback()
{
    keyboardMappingOpacity += keyboardMappingOpacityIncrement;

    if (keyboardMappingOpacity >= 1.f)
    {
        keyboardMappingOpacityIncrement = -keyboardMappingOpacityIncrement;
        keyboardMappingOpacity = 1.f;
    }
    else if (keyboardMappingOpacity <= 0.6f)
    {
        keyboardMappingOpacityIncrement = -keyboardMappingOpacityIncrement;
        keyboardMappingOpacity = 0.6f;
    }

    repaint();
}

bool VmpcTooltipComponent::isShowingKeyboardMapping()
{
    return keyboardMappingOpacity > 0.f;
}
