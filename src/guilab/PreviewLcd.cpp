#include "PreviewLcd.hpp"

#include "VmpcJuceResourceUtil.hpp"
#include "gui/vector/Constants.hpp"

using namespace vmpc_juce::guilab;
using vmpc_juce::gui::vector::Constants;

PreviewLcd::PreviewLcd()
    : pixels(juce::Image::PixelFormat::RGB, 248 * 2, 60 * 2, true)
{
    backlight.setColor(Constants::lcdOffBacklit.brighter().withAlpha(0.4f));

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
            pixelOn[static_cast<size_t>(x)][static_cast<size_t>(y)] = on;
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
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);

    constexpr float aspectRatio = 60.f / 248.f;
    const auto width = static_cast<float>(getWidth()) * magicMultiplier;
    const auto height = width * aspectRatio;
    const auto x = (static_cast<float>(getWidth()) - width) * 0.5f;
    const auto y = (static_cast<float>(getHeight()) - height) * 0.5f;
    const auto imageScale = width / static_cast<float>(pixels.getWidth());
    const auto transform =
        juce::AffineTransform().scaled(imageScale).translated(x, y);

    g.drawImageTransformed(pixels, transform);

    backlight.setRadius(std::round(static_cast<float>(getWidth()) / 248.f));
    juce::Path offPixels;
    for (size_t pixelX = 0; pixelX < pixelOn.size(); ++pixelX)
    {
        for (size_t pixelY = 0; pixelY < pixelOn[pixelX].size(); ++pixelY)
        {
            if (!pixelOn[pixelX][pixelY])
            {
                offPixels.addRectangle(static_cast<float>(pixelX * 2),
                                       static_cast<float>(pixelY * 2), 2.f,
                                       2.f);
            }
        }
    }

    offPixels.applyTransform(transform);
    backlight.render(g, offPixels);
}
