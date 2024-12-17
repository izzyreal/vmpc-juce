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
#pragma once

#include "juce_gui_basics/juce_gui_basics.h"
#include "hardware/HwComponent.hpp"

#include "Mpc.hpp"

// The reason we are using juce::Button instead of VmpcComponent
// or juce::Component, is we get tooltips for free.
class VmpcTooltipComponent : public juce::Button, public juce::Timer
{
public:
    VmpcTooltipComponent(mpc::Mpc&, std::shared_ptr<mpc::hardware::HwComponent> mpcHardwareComponent);

    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

    void paintButton(juce::Graphics&, bool, bool) override;

    void showKeyboardMapping();
    void hideKeyboardMapping();

    void timerCallback() override;

protected:
    bool isShowingKeyboardMapping();

    mpc::Mpc& mpc;

private:
    std::shared_ptr<mpc::hardware::HwComponent> mpcHardwareComponent;
    std::string keyboardMappingText;
    float keyboardMappingOpacity = 0.f;
    float keyboardMappingOpacityIncrement = 0.f;
};

/**
 * The mpc::hardware namespace is a little bit of a mess.
 * Only Button and HwPad extend HwComponent. This selection was made based on
 * which hardware can be associated with a keypress. The DataWheel is a bit of
 * an odd one, because it has 2 keys associated with it -- one for
 * anti-clockwise and one for clockwise.
 *
 * Probably the solution is to model the DataWheel as 2 individual components.
 * But for now I'll avoid that kind of refactoring and resort to the below adhoc
 * class whose sole purpose is to satisfy the arrangement in vmpc-juce, which is:
 * To show a keyboard shortcut tooltip, one must extend VmpcTooltipComponent and
 * provide it with a HwComponent.
 */
class DummyDataWheelHwComponent : public mpc::hardware::HwComponent {
public: explicit DummyDataWheelHwComponent(mpc::Mpc& _mpc) : mpc::hardware::HwComponent(_mpc, "") {}
};
