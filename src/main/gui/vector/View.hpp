#pragma once

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

class Keyboard;

namespace vmpc_juce::gui::vector
{

    class Lcd;
    class TooltipOverlay;
    class Menu;
    class Disclaimer;
    class About;
    class Pad;
    class Led;
    class DataWheel;
    class Pot;
    class SliderCap;

    class View final : public juce::Component, public juce::Timer
    {
    public:
        View(mpc::Mpc &mpcToUse,
             const std::function<void()> &showAudioSettingsDialog,
             juce::AudioProcessor::WrapperType wrapperType,
             const std::function<bool()> &isInstrument,
             bool &shouldShowDisclaimer);

        ~View() override;

        void resized() override;

        std::pair<int, int> getInitialRootWindowDimensions();

        float getAspectRatio() const;

        void timerCallback() override;

    private:
        void onKeyUp(int, bool ctrlDown, bool altDown, bool shiftDown) const;
        void onKeyDown(int, bool ctrlDown, bool altDown, bool shiftDown) const;
        mpc::Mpc &mpc;
        void deleteDisclaimer();
        std::string layoutName = "default_compact";
        std::vector<Component *> components;
        std::vector<MouseListener *> mouseListeners;
        node view_root;
        std::function<float()> getScale;
        const std::function<juce::Font &()> getMainFontScaled;
        const std::function<juce::Font &()> getMpc2000xlFaceplateGlyphsScaled;
        const std::function<juce::Font &()> getKeyTooltipFontScaled;

        focus::FocusHelper *focusHelper = nullptr;
        Keyboard *keyboard = nullptr;

        TooltipOverlay *tooltipOverlay = nullptr;
        Menu *menu = nullptr;
        Disclaimer *disclaimer = nullptr;
        About *about = nullptr;

        int base_width;
        int base_height;
        std::pair<int, int> initialRootWindowDimensions;

        std::vector<char> mainFontData;
        juce::Font mainFont;

        std::vector<char> mpc2000xlFaceplateGlyphsFontData;
        juce::Font mpc2000xlFaceplateGlyphsFont;

        std::vector<char> keyTooltipFontData;
        juce::Font keyTooltipFont;

        std::vector<Pad *> pads;
        std::vector<Led *> leds;
        DataWheel *dataWheel;
        Pot *recPot;
        Pot *volPot;
        SliderCap *sliderCap;

        Lcd *lcd;

        friend class Lcd;
    };

} // namespace vmpc_juce::gui::vector
