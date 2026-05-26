#pragma once

#include "AdditionalShadowComponentsProvider.hpp"
#include "AnimatedButtonStyle.hpp"
#include "gui/WithSharedTimerCallback.hpp"
#include "SvgComponent.hpp"
#include "hardware/Component.hpp"

#include <array>
#include <vector>

namespace vmpc_juce::gui::vector
{

    class CursorKeys final
        : public SvgComponent,
          public WithSharedTimerCallback,
          public AdditionalShadowComponentsProvider
    {
    public:
        class CursorButtonComponent final : public SvgComponent
        {
        public:
            CursorButtonComponent(
                const std::string &svgPath,
                juce::Component *commonParentWithShadowToUse,
                const float shadowSizeToUse,
                const std::function<float()> &getScaleToUse,
                const std::function<juce::Rectangle<float>()>
                    &getButtonBoundsToUse)
                : SvgComponent({svgPath}, commonParentWithShadowToUse,
                               shadowSizeToUse, getScaleToUse),
                  getButtonBounds(getButtonBoundsToUse)
            {
                setInterceptsMouseClicks(false, false);
            }

            juce::Path getShadowPath() override
            {
                return getDrawablePathWithin(getButtonBounds());
            }

            float getShadowSizeMultiplier() const override
            {
                return AnimatedButtonStyle::getShadowSizeMultiplier(
                    pressedVisual);
            }

            float getShadowDarknessMultiplier() const override
            {
                return AnimatedButtonStyle::getShadowDarknessMultiplier(
                    pressedVisual);
            }

            void paint(juce::Graphics &g) override
            {
                drawCurrentDrawableWithin(g, getButtonBounds());
            }

            void setPressedVisual(const bool shouldBePressed)
            {
                if (pressedVisual == shouldBePressed)
                {
                    return;
                }

                pressedVisual = shouldBePressed;
                repaint();

                if (shadow != nullptr)
                {
                    shadow->repaint();
                }
            }

            bool isPressedVisual() const
            {
                return pressedVisual;
            }

        private:
            const std::function<juce::Rectangle<float>()> getButtonBounds;
            bool pressedVisual = false;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CursorButtonComponent)
        };

        CursorKeys(const std::array<std::shared_ptr<mpc::hardware::Button>, 4>
                       &buttonsToTrack,
                   juce::Component *commonParentWithShadowToUse,
                   const std::function<float()> &getScaleToUse,
                   const float shadowSizeToUse)
            : SvgComponent({"cursor_keys_hole.svg"}, nullptr, 0.f,
                           getScaleToUse)
        {
            buttons[0].trackedButton = buttonsToTrack[0];
            buttons[0].component = new CursorButtonComponent(
                "cursor_keys_left_button.svg", commonParentWithShadowToUse,
                shadowSizeToUse, getScaleToUse,
                [this]()
                {
                    return getButtonBounds(0);
                });
            buttons[1].trackedButton = buttonsToTrack[1];
            buttons[1].component = new CursorButtonComponent(
                "cursor_keys_up_button.svg", commonParentWithShadowToUse,
                shadowSizeToUse, getScaleToUse,
                [this]()
                {
                    return getButtonBounds(1);
                });
            buttons[2].trackedButton = buttonsToTrack[2];
            buttons[2].component = new CursorButtonComponent(
                "cursor_keys_right_button.svg", commonParentWithShadowToUse,
                shadowSizeToUse, getScaleToUse,
                [this]()
                {
                    return getButtonBounds(2);
                });
            buttons[3].trackedButton = buttonsToTrack[3];
            buttons[3].component = new CursorButtonComponent(
                "cursor_keys_down_button.svg", commonParentWithShadowToUse,
                shadowSizeToUse, getScaleToUse,
                [this]()
                {
                    return getButtonBounds(3);
                });

            for (auto &button : buttons)
            {
                addAndMakeVisible(button.component);
            }
        }

        ~CursorKeys() override
        {
            for (auto &button : buttons)
            {
                delete button.component;
            }
        }

        std::vector<SvgComponent *> getAdditionalShadowComponents() override
        {
            std::vector<SvgComponent *> result;
            result.reserve(buttons.size());

            for (auto &button : buttons)
            {
                result.push_back(button.component);
            }

            return result;
        }

        void resized() override
        {
            for (auto &button : buttons)
            {
                button.component->setBounds(getLocalBounds());
                button.component->syncShadowSiblingSizeAndPosition();
            }
        }

        void moved() override
        {
            for (auto &button : buttons)
            {
                button.component->syncShadowSiblingSizeAndPosition();
            }
        }

        void paint(juce::Graphics &g) override
        {
            drawCurrentDrawableWithin(g, getLocalBounds().toFloat());
        }

        void sharedTimerCallback() override
        {
            for (auto &button : buttons)
            {
                const bool pressed = button.trackedButton != nullptr &&
                                     button.trackedButton->isPressed();
                button.component->setPressedVisual(pressed);
            }
        }

    private:
        struct CursorButton
        {
            std::shared_ptr<mpc::hardware::Button> trackedButton;
            CursorButtonComponent *component = nullptr;
        };

        juce::Rectangle<float> getButtonBounds(const size_t index)
        {
            const auto holeBounds = SvgComponent::getDrawableBounds();
            const auto buttonBounds =
                buttons[index].component->getDrawableBounds();

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
                        AnimatedButtonStyle::getPressedOffsetY(
                            buttons[index].component->isPressedVisual()),
                    relativeWidth * renderedHoleBounds.getWidth(),
                    relativeHeight * renderedHoleBounds.getHeight()};
        }
        std::array<CursorButton, 4> buttons;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CursorKeys)
    };

} // namespace vmpc_juce::gui::vector
