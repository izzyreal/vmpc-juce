#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <melatonin_blur/melatonin_blur.h>

#include "ViewUtil.hpp"

namespace vmpc_juce::gui::vector
{

    class Shadow : public juce::Component
    {

    public:
        Shadow(const std::function<float()> &getScaleToUse,
               const std::function<juce::Path()> getPathToUse,
               const float shadowSizeToUse, const float shadowDarknessToUse,
               const bool isInnerToUse)
            : getPath(getPathToUse), getScale(getScaleToUse),
              shadowSize(shadowSizeToUse), shadowDarkness(shadowDarknessToUse),
              isInner(isInnerToUse)
        {
            setInterceptsMouseClicks(false, false);

            if (isInner)
            {
                innerShadow.setColor(
                    juce::Colours::black.withAlpha(shadowDarkness));
            }
            else
            {
                dropShadow.setColor(
                    juce::Colours::black.withAlpha(shadowDarkness));
            }
        }

        void setDropShadowOffset(const juce::Point<float> &newOffset)
        {
            dropShadowOffset = newOffset;
        }

        void setDropShadowYScale(const float newScale)
        {
            dropShadowYScale = newScale;
        }

        void setClipDropShadowToSourceBottomWhenPressed(const bool shouldClip)
        {
            clipDropShadowToSourceBottomWhenPressed = shouldClip;
        }

        void setPressed(const bool newIsPressed)
        {
            isPressed = newIsPressed;
        }

        void setPressedBottomClipInset(const float newInset)
        {
            pressedBottomClipInset = newInset;
        }

        void setPressedBottomClipInsetFactor(const float newFactor)
        {
            pressedBottomClipInsetFactor = newFactor;
        }

        void setPressedDropShadowRadiusScale(const float newScale)
        {
            pressedDropShadowRadiusScale = newScale;
        }

        void setDropShadowSpread(const float newSpread)
        {
            dropShadowSpread = newSpread;
        }

        void setPressedDropShadowSpread(const float newSpread)
        {
            pressedDropShadowSpread = newSpread;
        }

        void paint(juce::Graphics &g) override
        {
            auto scale = getScale();
            auto radius = scale * shadowSize;

            if (!isInner && isPressed)
            {
                radius *= pressedDropShadowRadiusScale;
            }

            juce::Point<float> offset = {dropShadowOffset.x * scale * shadowSize,
                                         dropShadowOffset.y * scale * shadowSize};
            auto path = getPath();
            juce::AffineTransform transform;
            const auto shadowDimensions =
                ViewUtil::getShadowDimensions(shadowSize, scale);

            transform = transform.translated(shadowDimensions);
            path.applyTransform(transform);

            if (isInner)
            {
                innerShadow.setRadius(radius);
                innerShadow.setOffset(offset.withY(offset.getY() * 4));
                innerShadow.render(g, path);
            }
            else
            {
                if (dropShadowYScale != 1.f)
                {
                    const auto bounds = path.getBounds();
                    path.applyTransform(
                        juce::AffineTransform::scale(
                            1.f, dropShadowYScale, bounds.getCentreX(),
                            bounds.getCentreY()));
                }

                juce::Graphics::ScopedSaveState saveState(g);

                if (clipDropShadowToSourceBottomWhenPressed && isPressed)
                {
                    const auto sourceHeight =
                        getHeight() - static_cast<int>(shadowDimensions.y * 2.f);
                    const auto factorInset = static_cast<int>(std::ceil(
                        std::max(0.f, static_cast<float>(sourceHeight)) *
                        pressedBottomClipInsetFactor));
                    const auto clipHeight =
                        getHeight() - static_cast<int>(shadowDimensions.y) -
                        factorInset -
                        static_cast<int>(std::ceil(pressedBottomClipInset *
                                                   scale));
                    g.reduceClipRegion(
                        0, 0, getWidth(), std::max(0, clipHeight));
                }

                dropShadow.setColor(
                    juce::Colours::black.withAlpha(shadowDarkness));
                dropShadow.setRadius(radius);
                dropShadow.setOffset(offset);
                dropShadow.setSpread(
                    isPressed ? pressedDropShadowSpread : dropShadowSpread);
                dropShadow.render(g, path);
            }
        }

    private:
        melatonin::InnerShadow innerShadow;
        melatonin::DropShadow dropShadow;
        const std::function<juce::Path()> getPath;
        const std::function<float()> &getScale;
        const float shadowSize;
        const float shadowDarkness;
        const bool isInner;
        juce::Point<float> dropShadowOffset = {1.f, 0.1f};
        float dropShadowYScale = 1.f;
        bool clipDropShadowToSourceBottomWhenPressed = false;
        bool isPressed = false;
        float pressedBottomClipInset = 0.f;
        float pressedBottomClipInsetFactor = 0.f;
        float pressedDropShadowRadiusScale = 1.f;
        float dropShadowSpread = 0.f;
        float pressedDropShadowSpread = 0.f;
    };

} // namespace vmpc_juce::gui::vector
