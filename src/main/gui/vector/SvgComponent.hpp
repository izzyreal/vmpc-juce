#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_core/juce_core.h>

#include "ViewUtil.hpp"
#include "Shadow.hpp"
#include "VmpcJuceResourceUtil.hpp"

#include <map>
#include <optional>

namespace vmpc_juce::gui::vector
{

    class SvgComponent : public juce::Component
    {
    private:
        std::unique_ptr<juce::Drawable>
        createDrawable(const std::string svgPath)
        {
            std::vector<char> svgData =
                vmpc_juce::VmpcJuceResourceUtil::getResourceData("svg/" +
                                                                 svgPath);

            if (!svgData.empty())
            {
                const auto svgDataString =
                    juce::String(svgData.data(), svgData.size());

                auto svgXml = juce::XmlDocument::parse(svgDataString);

                if (svgXml != nullptr)
                {
                    const auto hashCode = juce::String(svgPath).hashCode();
                    randomColor = juce::Colour::fromRGB(
                        hashCode & 0xFF, (hashCode >> 8) & 0xFF,
                        (hashCode >> 16) & 0xFF);

                    return juce::Drawable::createFromSVG(*svgXml);
                }
            }

            return {};
        }

    public:
        SvgComponent(const std::vector<std::string> &svg_paths,
                     juce::Component *commonParentWithShadowToUse,
                     const float shadowSizeToUse,
                     const std::function<float()> &getScaleToUse,
                     const std::string svgPlacementToUse = "")
            : commonParentWithShadow(commonParentWithShadowToUse),
              shadowSize(shadowSizeToUse), getScale(getScaleToUse),
              svgPlacement(svgPlacementToUse)
        {
            if (svg_paths[0] == "display.svg" ||
                svg_paths[0] == "display_compact.svg")
            {
                setInterceptsMouseClicks(false, false);
            }

            for (auto &svg_path : svg_paths)
            {
                drawables[svg_path] = createDrawable(svg_path);
            }

            setSvgPath(svg_paths[0]);

            if (commonParentWithShadow != nullptr)
            {
                class ParentSizeAndPositionListener
                    : public juce::ComponentListener
                {
                public:
                    SvgComponent *svgComponent = nullptr;
                    void componentMovedOrResized(Component & /*component*/,
                                                 bool /*wasMoved*/,
                                                 bool /*wasResized*/)
                    {
                        svgComponent->syncShadowSiblingSizeAndPosition();
                    }
                };

                auto listener = new ParentSizeAndPositionListener();
                listener->svgComponent = this;
                parentSizeAndPositionListener = listener;
                commonParentWithShadow->addComponentListener(
                    parentSizeAndPositionListener);
            }
        }

        ~SvgComponent() override
        {
            if (commonParentWithShadow != nullptr)
            {
                commonParentWithShadow->removeComponentListener(
                    parentSizeAndPositionListener);
                delete parentSizeAndPositionListener;
            }
        }

        juce::Rectangle<float> getDrawableBounds()
        {
            return drawables[currentDrawable] == nullptr
                       ? juce::Rectangle<float>()
                       : drawables[currentDrawable]->getDrawableBounds();
        }

        juce::Path getShadowPath()
        {
            if (drawables[currentDrawable] == nullptr)
            {
                return juce::Path();
            }

            juce::Path shadowPath =
                drawables[currentDrawable]->getOutlineAsPath();
            auto centred =
                juce::RectanglePlacement(juce::RectanglePlacement::centred);
            auto transform = centred.getTransformToFit(
                getDrawableBounds(), getLocalBounds().toFloat());
            shadowPath.applyTransform(transform);
            return shadowPath;
        }

        void paint(juce::Graphics &g) override
        {
            // g.fillAll(randomColor);
            if (drawables[currentDrawable] == nullptr)
            {
                g.fillAll(juce::Colours::red);
                return;
            }

            drawables[currentDrawable]->drawWithin(
                g, getLocalBounds().toFloat(),
                svgPlacement == "stretched"
                    ? juce::RectanglePlacement::stretchToFit
                    : juce::RectanglePlacement::centred,
                1.0f);
        }

        juce::Component *shadow = nullptr;

        void setPressedShadowOffsetFactor(const float newFactor)
        {
            pressedShadowOffsetFactor = newFactor;
        }

        bool hasSvgPath(const std::string &svgPath) const
        {
            return drawables.find(svgPath) != drawables.end();
        }

        bool ensureSvgPathLoaded(const std::string &svgPath)
        {
            if (hasSvgPath(svgPath))
            {
                return drawables[svgPath] != nullptr;
            }

            drawables[svgPath] = createDrawable(svgPath);
            return drawables[svgPath] != nullptr;
        }

        bool setPressedAppearanceEnabled(const bool enabled)
        {
            if (enabled)
            {
                if (pressedBaseDrawable.has_value())
                {
                    return true;
                }

                const auto pressedPath =
                    getPressedVariantPath(currentDrawable);

                if (!pressedPath.has_value() ||
                    !ensureSvgPathLoaded(*pressedPath))
                {
                    return false;
                }

                pressedBaseDrawable = currentDrawable;
                setSvgPath(*pressedPath);
                if (auto *dropShadow = dynamic_cast<Shadow *>(shadow))
                {
                    dropShadow->setPressed(true);
                }
                syncShadowSiblingSizeAndPosition();
                return true;
            }

            if (!pressedBaseDrawable.has_value())
            {
                return false;
            }

            const auto unpressedPath = *pressedBaseDrawable;
            pressedBaseDrawable.reset();
            setSvgPath(unpressedPath);
            if (auto *dropShadow = dynamic_cast<Shadow *>(shadow))
            {
                dropShadow->setPressed(false);
            }
            syncShadowSiblingSizeAndPosition();
            return true;
        }

        void syncShadowSiblingSizeAndPosition()
        {
            if (shadow == nullptr)
            {
                return;
            }

            auto globalTopLeft = localPointToGlobal(juce::Point<int>(0, 0));
            auto relativeTopLeft =
                commonParentWithShadow->getLocalPoint(nullptr, globalTopLeft);
            juce::Rectangle<int> boundsInCommonParent(
                relativeTopLeft.x, relativeTopLeft.y, getWidth(), getHeight());

            const auto shadowDimensions =
                ViewUtil::getShadowDimensions(shadowSize, getScale());
            boundsInCommonParent.expand(shadowDimensions.x, shadowDimensions.y);

            if (pressedBaseDrawable.has_value() && pressedShadowOffsetFactor > 0.f)
            {
                boundsInCommonParent.translate(
                    0, static_cast<int>(std::round(getHeight() *
                                                   pressedShadowOffsetFactor)));
            }

            shadow->setBounds(boundsInCommonParent);
        }

        void resized() override
        {
            syncShadowSiblingSizeAndPosition();
        }

        void moved() override
        {
            syncShadowSiblingSizeAndPosition();
        }

    protected:
        void setSvgPath(const std::string newSvgPath)
        {
            currentDrawable = newSvgPath;
            repaint();
        }

        juce::Drawable *getCurrentDrawable()
        {
            return drawables[currentDrawable].get();
        }

    private:
        static std::optional<std::string>
        getPressedVariantPath(const std::string &svgPath)
        {
            constexpr auto suffix = ".svg";
            if (svgPath.size() <= 4 ||
                svgPath.substr(svgPath.size() - 4) != suffix)
            {
                return std::nullopt;
            }

            return svgPath.substr(0, svgPath.size() - 4) + "_pressed.svg";
        }

        std::map<std::string, std::unique_ptr<juce::Drawable>> drawables;
        std::string currentDrawable;
        std::optional<std::string> pressedBaseDrawable;
        float pressedShadowOffsetFactor = 0.f;
        juce::Colour randomColor;
        juce::Component *commonParentWithShadow = nullptr;
        juce::ComponentListener *parentSizeAndPositionListener = nullptr;
        const float shadowSize;
        const std::function<float()> &getScale;
        const std::string svgPlacement;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SvgComponent)
    };

} // namespace vmpc_juce::gui::vector
