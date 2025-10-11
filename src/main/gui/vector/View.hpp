#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include "Node.hpp"

#include <functional>

namespace mpc { class Mpc; }

class Keyboard;

namespace vmpc_juce::gui::vector {

    class Lcd;
    class TooltipOverlay;
    class Menu;
    class Disclaimer;
    class About;
    class Pad;

    class View : public juce::Component, public juce::Timer {

        public:
            View(mpc::Mpc &mpc,
                 const std::function<void()> &showAudioSettingsDialog,
                 const juce::AudioProcessor::WrapperType wrapperType,
                 const std::function<bool()> isInstrument,
                 bool &shouldShowDisclaimer);
                    
            ~View() override;

            void resized() override;

            const std::pair<int, int> getInitialRootWindowDimensions();

            const float getAspectRatio();

            void timerCallback() override;

        private:
            void onKeyUp(int, bool ctrlDown, bool altDown, bool shiftDown);
            void onKeyDown(int, bool ctrlDown, bool altDown, bool shiftDown);
            mpc::Mpc &mpc;
            void deleteDisclaimer();
            std::string name = "default_compact";
            std::vector<juce::Component*> components;
            std::vector<juce::MouseListener*> mouseListeners;
            node view_root;
            std::function<float()> getScale;
            const std::function<juce::Font&()> getMainFontScaled;
            const std::function<juce::Font&()> getMpc2000xlFaceplateGlyphsScaled;
            const std::function<juce::Font&()> getKeyTooltipFontScaled;
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

            std::vector<Pad*> pads;

            friend class Lcd;
    };

} // namespace vmpc_juce::gui::vector
