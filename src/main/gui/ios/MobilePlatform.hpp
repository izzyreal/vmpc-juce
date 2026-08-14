#pragma once

#include "gui/arrangement/ArrangementModel.hpp"

namespace vmpc_juce::gui::ios
{
    bool isRunningOnIPhone();
    void setIPhoneStatusBarHidden(bool hidden);
    void setIPhoneOrientation(arrangement::Orientation orientation);
} // namespace vmpc_juce::gui::ios
