#pragma once

#include <juce_audio_plugin_client/juce_audio_plugin_client.h>

#include <vector>
#include <memory>

namespace mpc { class Mpc; }

class VmpcParameterListener : public juce::AudioProcessorParameter::Listener {
private:
    mpc::Mpc& mpc;

public:
    explicit VmpcParameterListener(mpc::Mpc&);

    ~VmpcParameterListener() override = default;

    void parameterValueChanged(int parameterIndex, float newValue) override;

    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;
};

class VmpcParameters {

private:
    std::vector<juce::AudioProcessorParameter*> parameters;
    std::unique_ptr<VmpcParameterListener> listener;

public:
    explicit VmpcParameters(mpc::Mpc&);

    const std::vector<juce::AudioProcessorParameter*>& getParameters();
};