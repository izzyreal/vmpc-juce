#pragma once

#include "juce_graphics/juce_graphics.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_core/juce_core.h>

#include "ViewUtil.hpp"
#include "ResourceUtil.h"

class SvgComponent : public juce::Component
{
    private:
        void loadSvgFile(const std::string svgPath)
        {
            const auto svgData = mpc::ResourceUtil::get_resource_data("svg/" + svgPath);

            if (!svgData.empty())
            {  
                const auto svgDataString = juce::String(juce::CharPointer_UTF8(svgData.data()));

                auto svgXml = juce::XmlDocument::parse(svgDataString);
                
                if (svgXml != nullptr)
                {   
                    svgDrawable = juce::Drawable::createFromSVG(*svgXml);
                    svgDrawable->getDrawableBounds();

                    const auto hashCode = juce::String(svgPath).hashCode();
                    randomColor = juce::Colour::fromRGB(hashCode & 0xFF,
                            (hashCode >> 8) & 0xFF,
                            (hashCode >> 16) & 0xFF);
                }
            }
        }

    public:
        SvgComponent(std::string svg_path, juce::Component *commonParentWithShadowToUse, const float shadowSizeToUse, const std::function<float()> &getScaleToUse)
            : commonParentWithShadow(commonParentWithShadowToUse), shadowSize(shadowSizeToUse), getScale(getScaleToUse)
        {
            if (svg_path == "display.svg" || svg_path == "display_compact.svg") setInterceptsMouseClicks(false, false);

            setSvgPath(svg_path);

            if (commonParentWithShadow != nullptr)
            {
                class ParentSizeAndPositionListener : public juce::ComponentListener {
                    public:
                        SvgComponent *svgComponent = nullptr;
                        void componentMovedOrResized (Component &component, bool wasMoved, bool wasResized) {
                            svgComponent->syncShadowSiblingSizeAndPosition();
                        }
                };

                auto listener = new ParentSizeAndPositionListener();
                listener->svgComponent = this;
                parentSizeAndPositionListener = listener;
                commonParentWithShadow->addComponentListener(parentSizeAndPositionListener);
            }
        }

        ~SvgComponent() override
        {
            if (commonParentWithShadow !=  nullptr)
            {
                commonParentWithShadow->removeComponentListener(parentSizeAndPositionListener);
                delete parentSizeAndPositionListener;
            }
        }

        juce::Rectangle<float> getDrawableBounds()
        {
            return svgDrawable == nullptr ? juce::Rectangle<float>() : svgDrawable->getDrawableBounds();
        }

        juce::Path getShadowPath()
        {
            if (svgDrawable == nullptr) return juce::Path();

            juce::Path shadowPath = svgDrawable->getOutlineAsPath();
            auto centred = juce::RectanglePlacement(juce::RectanglePlacement::centred);
            auto transform = centred.getTransformToFit(getDrawableBounds(), getLocalBounds().toFloat());
            shadowPath.applyTransform(transform);
            return shadowPath;
        }

        void paint(juce::Graphics& g) override
        {
            //g.fillAll(randomColor);
            if (svgDrawable == nullptr)
            {
                g.fillAll(juce::Colours::red);
                return;
            }

            svgDrawable->drawWithin(g, getLocalBounds().toFloat(), juce::RectanglePlacement::centred, 1.0f);
        }

        juce::Component *shadow = nullptr;

        void syncShadowSiblingSizeAndPosition()
        {
            if (shadow == nullptr) return;

            auto globalTopLeft = localPointToGlobal(juce::Point<int>(0, 0));
            auto relativeTopLeft = commonParentWithShadow->getLocalPoint(nullptr, globalTopLeft);
            juce::Rectangle<int> boundsInCommonParent(relativeTopLeft.x, relativeTopLeft.y, getWidth(), getHeight()); 

            const auto shadowDimensions = ViewUtil::getShadowDimensions(shadowSize, getScale());
            boundsInCommonParent.expand(shadowDimensions.x, shadowDimensions.y);

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
        std::unique_ptr<juce::Drawable> svgDrawable;

        void setSvgPath(const std::string newSvgPath)
        {
            loadSvgFile(newSvgPath);
            repaint();
        }

    private:
        juce::Colour randomColor;
        juce::Component *commonParentWithShadow = nullptr;
        juce::ComponentListener *parentSizeAndPositionListener = nullptr;
        const float shadowSize;
        const std::function<float()> &getScale;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SvgComponent)
};
