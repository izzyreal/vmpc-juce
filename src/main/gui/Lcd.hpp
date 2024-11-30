#pragma once
#include "juce_graphics/juce_graphics.h"
#include <juce_gui_basics/juce_gui_basics.h>

#include "Constants.hpp"
#include "melatonin_blur/melatonin/shadows.h"

class Lcd : public juce::Component {
    public:
        void paint(juce::Graphics &g) override
        {
            g.fillAll(Constants::lcdOffBacklit);
            g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
            std::vector<std::vector<bool>> rawPixels(60, std::vector<bool>(248));

            juce::Path p;
            juce::Image img(juce::Image::PixelFormat::RGB, 248*2, 60*2, false);

            const auto asp_ratio = 60.f/248.f;
            const auto w = float(getWidth()) * magicMultiplier;
            const auto h = w * asp_ratio;

            const auto img_scale = w / (248 * 2);
            const auto unused_h_px = getWidth() - w;
            const auto unused_v_px = getHeight() - h;
            const auto x_offset = unused_h_px * 0.5f;
            const auto y_offset = unused_v_px * 0.5f;

            for (uint8_t y = 0; y < 60; y++)
            {
                for (uint8_t x = 0; x < 248; x++)
                {
                    const bool on = rawPixels[y][x];
                    drawLcdPixel(img, x, y, on);
                    if (!on) p.addRectangle(x*2, y*2, 2, 2);
                }
            }

            auto color = Constants::lcdOffBacklit.brighter().withAlpha(0.4f);
            int radius = (int) std::round<float>((float)getWidth() / 248.f);
            juce::Point<int> offset = { 0, 0 };
            int spread = 0.f;
            melatonin::DropShadow shadow = { color, radius, offset, spread };
            
            juce::AffineTransform t;
            t = t.scaled(img_scale);
            t = t.translated(x_offset, y_offset);

            p.applyTransform(t);
            g.drawImageTransformed(img, t);

            shadow.render(g, p);
        }

        float magicMultiplier = 0.55f;

    private:
        void drawLcdPixel(juce::Image &img, const uint8_t lcdX, const uint8_t lcdY, const bool on)
        {
            const juce::Colour c1 = on ? Constants::lcdOn : Constants::lcdOffBacklit;
            const juce::Colour c2 = on ? Constants::lcdOnLight : Constants::lcdOffBacklit;
            img.setPixelAt(lcdX*2, lcdY*2, c1);
            img.setPixelAt(lcdX*2 + 1, lcdY*2, c2);
            img.setPixelAt(lcdX*2 + 1, lcdY*2 + 1, c2);
            img.setPixelAt(lcdX*2, lcdY*2 + 1, c2);
        }
};

