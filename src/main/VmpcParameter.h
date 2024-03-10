#pragma once

#include "hardware/HwComponent.hpp"

#include <juce_audio_plugin_client/juce_audio_plugin_client.h>

class VmpcParameter : public juce::AudioParameterInt {
public:
    VmpcParameter(mpc::hardware::HwComponent& component);
    mpc::hardware::HwComponent& component;
};