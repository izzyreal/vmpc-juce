#pragma once

#include "gui/WithSharedTimerCallback.hpp"
#include "Shadow.hpp"
#include "SvgComponent.hpp"
#include "hardware/Component.hpp"

#include <array>

namespace vmpc_juce::gui::vector
{

    class CursorKeys final : public SvgComponent, public WithSharedTimerCallback
    {
    public:
        CursorKeys(const std::array<std::shared_ptr<mpc::hardware::Button>, 4>
                       &buttonsToTrack,
                   juce::Component *commonParentWithShadowToUse,
                   const std::function<float()> &getScaleToUse,
                   const float shadowSizeToUse)
            : SvgComponent({"cursor_keys_hole.svg"}, nullptr, 0.f,
                           getScaleToUse),
              commonParentWithShadow(commonParentWithShadowToUse),
              getScale(getScaleToUse), shadowSize(shadowSizeToUse)
        {
            buttons[0].trackedButton = buttonsToTrack[0];
            buttons[0].svg = new SvgComponent({"cursor_keys_left_button.svg"},
                                              nullptr, 0.f, getScaleToUse);
            buttons[1].trackedButton = buttonsToTrack[1];
            buttons[1].svg = new SvgComponent({"cursor_keys_up_button.svg"},
                                              nullptr, 0.f, getScaleToUse);
            buttons[2].trackedButton = buttonsToTrack[2];
            buttons[2].svg = new SvgComponent({"cursor_keys_right_button.svg"},
                                              nullptr, 0.f, getScaleToUse);
            buttons[3].trackedButton = buttonsToTrack[3];
            buttons[3].svg = new SvgComponent({"cursor_keys_down_button.svg"},
                                              nullptr, 0.f, getScaleToUse);

            for (size_t i = 0; i < buttons.size(); i++)
            {
                const auto getPath = [this, i]() -> juce::Path
                {
                    return buttons[i].svg->getDrawablePathWithin(
                        getButtonBounds(i));
                };
                const auto getShadowSizeMultiplier = [this, i]() -> float
                {
                    return buttons[i].pressedVisual ? 0.8f : 1.f;
                };
                const auto getShadowDarknessMultiplier = [this, i]() -> float
                {
                    return buttons[i].pressedVisual ? 0.8f : 1.f;
                };

                buttons[i].shadow =
                    new Shadow(getScale, getPath, getShadowSizeMultiplier,
                               getShadowDarknessMultiplier, shadowSize, 0.4f,
                               false);
                commonParentWithShadow->addAndMakeVisible(buttons[i].shadow);
            }
        }

        ~CursorKeys() override
        {
            for (auto &button : buttons)
            {
                delete button.shadow;
                delete button.svg;
            }
        }

        void resized() override
        {
            syncShadowSiblingSizeAndPosition();
        }

        void moved() override
        {
            syncShadowSiblingSizeAndPosition();
        }

        void paint(juce::Graphics &g) override
        {
            drawCurrentDrawableWithin(g, getLocalBounds().toFloat());
        }

        void paintOverChildren(juce::Graphics &g) override
        {
            for (size_t i = 0; i < buttons.size(); i++)
            {
                buttons[i].svg->drawCurrentDrawableWithin(g, getButtonBounds(i));
            }
        }

        void sharedTimerCallback() override
        {
            bool anyChanged = false;

            for (auto &button : buttons)
            {
                const bool pressed = button.trackedButton != nullptr &&
                                     button.trackedButton->isPressed();

                if (button.pressedVisual == pressed)
                {
                    continue;
                }

                button.pressedVisual = pressed;
                button.shadow->repaint();
                anyChanged = true;
            }

            if (anyChanged)
            {
                repaint();
            }
        }

    private:
        struct CursorButton
        {
            std::shared_ptr<mpc::hardware::Button> trackedButton;
            SvgComponent *svg = nullptr;
            Shadow *shadow = nullptr;
            bool pressedVisual = false;
        };

        juce::Rectangle<float> getButtonBounds(const size_t index)
        {
            const auto holeBounds = SvgComponent::getDrawableBounds();
            const auto buttonBounds = buttons[index].svg->getDrawableBounds();

            if (holeBounds.isEmpty() || buttonBounds.isEmpty())
            {
                return getLocalBounds().toFloat();
            }

            const auto renderedHoleBounds =
                holeBounds.transformedBy(
                    getCurrentDrawableTransformWithin(getLocalBounds().toFloat()));

            const auto relativeX =
                (buttonBounds.getX() - holeBounds.getX()) / holeBounds.getWidth();
            const auto relativeY = (buttonBounds.getY() - holeBounds.getY()) /
                                   holeBounds.getHeight();
            const auto relativeWidth =
                buttonBounds.getWidth() / holeBounds.getWidth();
            const auto relativeHeight =
                buttonBounds.getHeight() / holeBounds.getHeight();

            return {renderedHoleBounds.getX() +
                        (relativeX * renderedHoleBounds.getWidth()),
                    renderedHoleBounds.getY() +
                        (relativeY * renderedHoleBounds.getHeight()) +
                        (buttons[index].pressedVisual ? 0.7f : 0.f),
                    relativeWidth * renderedHoleBounds.getWidth(),
                    relativeHeight * renderedHoleBounds.getHeight()};
        }

        juce::Rectangle<int> getBoundsInCommonParent() const
        {
            auto globalTopLeft = localPointToGlobal(juce::Point<int>(0, 0));
            auto relativeTopLeft =
                commonParentWithShadow->getLocalPoint(nullptr, globalTopLeft);
            return {relativeTopLeft.x, relativeTopLeft.y, getWidth(), getHeight()};
        }

        void syncShadowSiblingSizeAndPosition()
        {
            const auto shadowDimensions =
                ViewUtil::getShadowDimensions(shadowSize, getScale());
            auto boundsInCommonParent = getBoundsInCommonParent();
            boundsInCommonParent.expand(shadowDimensions.x, shadowDimensions.y);

            for (auto &button : buttons)
            {
                button.shadow->setBounds(boundsInCommonParent);
            }
        }

        juce::Component *commonParentWithShadow = nullptr;
        const std::function<float()> &getScale;
        const float shadowSize;
        std::array<CursorButton, 4> buttons;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CursorKeys)
    };

} // namespace vmpc_juce::gui::vector
