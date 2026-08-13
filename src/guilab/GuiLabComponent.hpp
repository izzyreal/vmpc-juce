#pragma once

#include "gui/vector/Node.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace vmpc_juce::guilab
{
    struct CatalogEntry
    {
        const char *id;
        const char *title;
        const char *resourceName;
        int width;
        int height;
    };

    class PreviewComponent final : public juce::Component
    {
    public:
        explicit PreviewComponent(const CatalogEntry &entry);
        ~PreviewComponent() override;

        void paint(juce::Graphics &g) override;
        void resized() override;
        void setHardwareScale(float newScale);

    private:
        float hardwareScale = 2.f;
        gui::vector::node root;
        std::vector<juce::Component *> components;
        std::string errorMessage;

        std::vector<char> mainFontData;
        std::vector<char> faceplateFontData;
        juce::Font mainFont;
        juce::Font faceplateFont;

        std::function<float()> getScale;
        std::function<juce::Font &()> getMainFontScaled;
        std::function<juce::Font &()> getFaceplateFontScaled;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PreviewComponent)
    };

    class GuiLabComponent final : public juce::Component
    {
    public:
        GuiLabComponent();
        ~GuiLabComponent() override;

        void resized() override;
        void paint(juce::Graphics &g) override;

    private:
        class PreviewCard;

        juce::Label heading;
        juce::Viewport viewport;
        juce::Component gallery;
        std::vector<std::unique_ptr<PreviewCard>> cards;

        void layoutGallery();

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuiLabComponent)
    };
} // namespace vmpc_juce::guilab
