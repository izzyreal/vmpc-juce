#include "PreviewLcd.hpp"

#include "VmpcJuceResourceUtil.hpp"
#include "gui/vector/Constants.hpp"

using namespace vmpc_juce::guilab;
using vmpc_juce::gui::vector::Constants;

PreviewLcd::PreviewLcd()
    : pixels(juce::Image::PixelFormat::RGB, 248 * 2, 60 * 2, true)
{
    const auto source =
        vmpc_juce::VmpcJuceResourceUtil::loadImage("screens/bg/sequencer.png");

    juce::Graphics imageGraphics(pixels);
    imageGraphics.fillAll(Constants::lcdOff);

    for (int y = 0; y < 50; ++y)
    {
        for (int x = 0; x < 248; ++x)
        {
            const bool on =
                source.isValid() &&
                source.getPixelAt(x, y).getPerceivedBrightness() < 0.5f;
            const auto primary = on ? Constants::lcdOn : Constants::lcdOff;
            const auto secondary =
                on ? Constants::lcdOnLight : Constants::lcdOff;

            pixels.setPixelAt(x * 2, y * 2, primary);
            pixels.setPixelAt(x * 2 + 1, y * 2, secondary);
            pixels.setPixelAt(x * 2, y * 2 + 1, secondary);
            pixels.setPixelAt(x * 2 + 1, y * 2 + 1, secondary);
        }
    }
}

void PreviewLcd::paint(juce::Graphics &g)
{
    g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);

    constexpr float aspectRatio = 60.f / 248.f;
    const auto width = static_cast<float>(getWidth()) * magicMultiplier;
    const auto height = width * aspectRatio;
    const auto x = (static_cast<float>(getWidth()) - width) * 0.5f;
    const auto y = (static_cast<float>(getHeight()) - height) * 0.5f;

    g.drawImage(pixels, {x, y, width, height});
}
