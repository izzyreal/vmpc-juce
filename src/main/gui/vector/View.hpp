#pragma once

#include "gui/WithSharedTimerCallback.hpp"
#include "gui/arrangement/ArrangementModel.hpp"

#include <juce_gui_basics/juce_gui_basics.h>

#include "gui/vector/Node.hpp"

#include <juce_audio_utils/juce_audio_utils.h>

#include <functional>

namespace mpc
{
    class Mpc;
}

namespace vmpc_juce::gui::focus
{
    class FocusHelper;
}

namespace vmpc_juce::gui::arrangement
{
    class ArrangementSurface;
    class ArrangementSelectorOverlay;
} // namespace vmpc_juce::gui::arrangement

class Keyboard;

namespace vmpc_juce::gui::vector
{
    class TooltipOverlay;
    class Menu;
    class Disclaimer;
    class About;
    class Lcd;
    class Pad;

    class View final : public juce::Component, public juce::Timer
    {
    public:
        View(mpc::Mpc &mpcToUse,
             const std::function<void()> &showAudioSettingsDialog,
             juce::AudioProcessor::WrapperType wrapperType,
             const std::function<bool()> &isInstrument,
             bool &shouldShowDisclaimer,
             const std::optional<std::string> &preferredArrangementId,
             std::function<void(const std::string &)> arrangementSelected,
             bool menuExpanded, std::function<void(bool)> menuExpandedChanged);

        ~View() override;

        void resized() override;
        void paint(juce::Graphics &) override;

        std::pair<int, int> getInitialRootWindowDimensions();

        float getAspectRatio() const;

        void timerCallback() override;

        focus::FocusHelper *getFocusHelper() const;

        Keyboard *getKeyboard() const;
        bool usesPhoneArrangements() const;
        void
        restoreArrangement(const std::optional<std::string> &arrangementId);
        void restoreMenuExpanded(bool expanded);

    private:
        void onKeyUp(int, bool ctrlDown, bool altDown, bool shiftDown) const;
        void onKeyDown(int, bool ctrlDown, bool altDown, bool shiftDown) const;
        mpc::Mpc &mpc;
        void deleteDisclaimer();
        void buildPhoneArrangement();
        void refreshHardwareRegistrations();
        void showArrangementSelector();
        void closeArrangementSelector();
        Lcd *findRenderedLcd();
        bool toggleAuxLcdWindow();
        void closeAuxLcdWindow();
        void selectArrangementSlot(std::size_t index,
                                   bool reportSelection = true);
        void toggleIPhoneFullscreen();
        std::string layoutName = "default_compact";
        std::vector<Component *> components;
        std::vector<MouseListener *> mouseListeners;
        node view_root;
        std::function<float()> getScale;
        const std::function<juce::Font &()> getMainFontScaled;
        const std::function<juce::Font &()> getMpc2000xlFaceplateGlyphsScaled;
        const std::function<juce::Font &()> getKeyTooltipFontScaled;

        std::function<void()> closeAbout;
        std::function<void(const std::string &)> arrangementSelected;
        std::function<void(bool)> menuExpandedChanged;

        focus::FocusHelper *focusHelper = nullptr;
        Keyboard *keyboard = nullptr;

        TooltipOverlay *tooltipOverlay = nullptr;
        Menu *menu = nullptr;
        Disclaimer *disclaimer = nullptr;
        About *about = nullptr;

        Timer *padTimer = nullptr;
        std::vector<Pad *> pads;

        int base_width;
        int base_height;
        std::pair<int, int> initialRootWindowDimensions;

        std::vector<char> mainFontData;
        juce::Font mainFont;

        std::vector<char> mpc2000xlFaceplateGlyphsFontData;
        juce::Font mpc2000xlFaceplateGlyphsFont;

        std::vector<char> keyTooltipFontData;
        juce::Font keyTooltipFont;

        std::vector<WithSharedTimerCallback *> timerCallbackComponents;

        bool phoneArrangementMode = false;
        bool iPhoneStatusBarHidden = false;
        juce::AudioProcessor::WrapperType processorWrapperType;
        gui::arrangement::ArrangementSetup arrangementSetup;
        std::size_t activeArrangementSlot = 0;
        arrangement::ArrangementSurface *arrangementSurface = nullptr;
        arrangement::ArrangementSelectorOverlay *arrangementSelector = nullptr;
        std::string arrangementError;
    };

} // namespace vmpc_juce::gui::vector
