#pragma once

#include "ArrangementModel.hpp"
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
        float referenceWidth;
        float referenceHeight;
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

    class GuiLabComponent final : public juce::Component,
                                  public juce::DragAndDropContainer
    {
    public:
        GuiLabComponent();
        ~GuiLabComponent() override;

        void resized() override;
        void paint(juce::Graphics &g) override;

    private:
        class PreviewCard;
        class ArrangementWorkspace;

        juce::Label heading;
        juce::TextButton loadButton{"Load"};
        juce::TextButton saveButton{"Save"};
        juce::Label brandLabel;
        juce::Label deviceLabel;
        juce::Label orientationLabel;
        juce::ComboBox brandSelector;
        juce::ComboBox deviceSelector;
        juce::ComboBox orientationSelector;
        juce::Label paletteHeading;
        juce::Viewport paletteViewport;
        juce::Component paletteGallery;
        std::vector<std::unique_ptr<PreviewCard>> cards;
        std::vector<const DeviceProfile *> visibleDevices;
        std::unique_ptr<ArrangementWorkspace> workspace;
        std::unique_ptr<juce::FileChooser> designFileChooser;
        juce::File currentDesignFile;

        void configureControls();
        void populateDevices(const std::string &preferredDeviceId = {});
        void updateTarget();
        void layoutPalette();
        void chooseDesignToLoad();
        void chooseDesignToSave();
        void loadDesignFile(const juce::File &file);
        void saveDesignFile(juce::File file);
        void showFileError(const juce::String &message);

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuiLabComponent)
    };
} // namespace vmpc_juce::guilab
