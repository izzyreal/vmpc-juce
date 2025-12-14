#include "standalone/AudioMidiSettingsComponent.hpp"

using namespace vmpc_juce::standalone;

AudioMidiSettingsComponent::AudioMidiSettingsComponent(
    juce::AudioDeviceManager &deviceManagerToUse,
    const int maxAudioInputChannels, const int maxAudioOutputChannels)
    : deviceSelector(deviceManagerToUse, 0, maxAudioInputChannels, 0,
                     maxAudioOutputChannels)
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
    const juce::ScopedValueSetter scope(isResizing, true);

    const auto r = getLocalBounds();

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
