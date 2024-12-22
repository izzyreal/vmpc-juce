#include <juce_gui_basics/juce_gui_basics.h>

#include "Node.hpp"

#include <functional>

namespace mpc { class Mpc; }

class Keyboard;

namespace vmpc_juce::gui::vector {

    class Lcd;
    class LedController;
    class TooltipOverlay;
    class Menu;
    class Disclaimer;
    class About;

    class View : public juce::Component {

        public:
            View(mpc::Mpc &mpc, const std::function<void()> &showAudioSettingsDialog);
                    
            ~View() override;

            void resized() override;

            const std::pair<int, int> getInitialRootWindowDimensions();

            const float getAspectRatio();

        private:
            void onKeyUp(int);
            void onKeyDown(int);
            mpc::Mpc &mpc;
            void deleteDisclaimer();
            std::string name = "default_compact";
            std::vector<juce::Component*> components;
            std::vector<juce::MouseListener*> mouseListeners;
            node view_root;
            std::function<float()> getScale;
            const std::function<juce::Font&()> getMainFontScaled;
            const std::function<juce::Font&()> getMpc2000xlFaceplateGlyphsScaled;
            Keyboard *keyboard = nullptr;
            LedController *ledController = nullptr;
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

            friend class Lcd;
    };

} // namespace vmpc_juce::gui::vector
