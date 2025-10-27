#include "AudioMidiSettingsComponent.hpp"

using namespace vmpc_juce;

AudioMidiSettingsComponent::AudioMidiSettingsComponent(
    juce::AudioDeviceManager &deviceManagerToUse, int maxAudioInputChannels,
    int maxAudioOutputChannels)
    : deviceSelector(deviceManagerToUse, 0, maxAudioInputChannels, 0,
                     maxAudioOutputChannels, true, true, true, false)
{
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
    const juce::ScopedValueSetter<bool> scope(isResizing, true);

    auto r = getLocalBounds();

    deviceSelector.setBounds(r);
}

void AudioMidiSettingsComponent::childBoundsChanged(juce::Component *childComp)
{
    if (!isResizing && childComp == &deviceSelector)
    {
        setToRecommendedSize();
    }
}

void AudioMidiSettingsComponent::setToRecommendedSize()
{
    setSize(getWidth(), deviceSelector.getHeight());
}
