#include "standalone/AudioMidiSettingsComponent.hpp"

#include "standalone/AudioDeviceManager.hpp"

using namespace vmpc_juce::standalone;

AudioMidiSettingsComponent::AudioMidiSettingsComponent(
    AudioDeviceManager &deviceManagerToUse,
    const int maxAudioInputChannels, const int maxAudioOutputChannels)
    : deviceSelector(deviceManagerToUse, 0, maxAudioInputChannels, 0,
                     maxAudioOutputChannels)
{
    juce::Font::setDefaultMinimumHorizontalScaleFactor(1.0f);
    setOpaque(true);
    addAndMakeVisible(deviceSelector);
}

void AudioMidiSettingsComponent::paint(juce::Graphics &g)
{
    g.fillAll(
        getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void AudioMidiSettingsComponent::resized()
{
    const juce::ScopedValueSetter scope(isResizing, true);

    const auto r = getLocalBounds();

    deviceSelector.setBounds(r);
}
