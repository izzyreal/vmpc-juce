#pragma once

#include "gui/arrangement/ArrangementCatalog.hpp"
#include "gui/arrangement/ArrangementModel.hpp"
#include "gui/vector/Node.hpp"

#include <juce_data_structures/juce_data_structures.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <array>
#include <memory>
#include <string>
#include <vector>

namespace vmpc_juce::guilab
{
    using namespace gui::arrangement;

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
        explicit GuiLabComponent(juce::PropertiesFile &settingsToUse);
        ~GuiLabComponent() override;

        void resized() override;
        void paint(juce::Graphics &g) override;
        bool keyPressed(const juce::KeyPress &key) override;

    private:
        class PreviewCard;
        class ArrangementWorkspace;

        juce::PropertiesFile &settings;
        juce::Label heading;
        juce::TextButton loadButton{"Load"};
        juce::TextButton saveButton{"Save"};
        juce::TextButton saveSetupButton{"Save Setup"};
        juce::TextButton clearSlotButton{"Clear Slot"};
        juce::Label slotsLabel;
        std::array<juce::TextButton, ArrangementSetup::slotCount> slotButtons{
            juce::TextButton{"1"}, juce::TextButton{"2"}, juce::TextButton{"3"},
            juce::TextButton{"4"}, juce::TextButton{"5"}};
        juce::Label brandLabel;
        juce::Label deviceLabel;
        juce::Label orientationLabel;
        juce::ComboBox brandSelector;
        juce::ComboBox deviceSelector;
        juce::ComboBox orientationSelector;
        juce::ToggleButton menuAtTopToggle{"Menu at top"};
        juce::Label paletteHeading;
        juce::Viewport paletteViewport;
        juce::Component paletteGallery;
        std::vector<std::unique_ptr<PreviewCard>> cards;
        std::vector<const DeviceProfile *> visibleDevices;
        std::unique_ptr<ArrangementWorkspace> workspace;
        std::unique_ptr<juce::FileChooser> fileChooser;
        juce::File currentDesignFile;
        juce::File currentSetupFile;
        ArrangementSetup setup;
        std::size_t activeSlot = 0;

        void configureControls();
        void populateDevices(const std::string &preferredDeviceId = {});
        void updateTarget();
        void layoutPalette();
        void chooseFileToLoad();
        void chooseDesignToSave();
        void loadFile(const juce::File &file);
        void saveCurrentSetup();
        void chooseSetupToSave();
        void restoreRecentDocument();
        juce::File getRememberedFile(const juce::String &key) const;
        void rememberRecentFile(const juce::File &file, bool isSetup);
        void loadDesignFile(const juce::File &file);
        void saveDesignFile(juce::File file);
        void loadSetupFile(const juce::File &file);
        void saveSetupFile(juce::File file);
        void persistActiveSlot(bool replaceIdentity = false);
        void selectSlot(std::size_t index);
        void clearActiveSlot();
        void updateSlotButtons();
        void syncArrangementControls();
        void setFileButtonsEnabled(bool enabled);
        void showFileError(const juce::String &message);

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuiLabComponent)
    };
} // namespace vmpc_juce::guilab
