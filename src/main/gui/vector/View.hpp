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
            View(mpc::Mpc &mpc,
                    const std::function<float()>& getScale,
                    const std::function<juce::Font&()> &getNimbusSansScaled,
                    const std::function<juce::Font&()> &getMpc2000xlFaceplateGlyphsScaled,
                    const std::function<void()> &showAudioSettingsDialog,
                    const std::function<void()> &resetWindowSize);
            ~View() override;

            void resized() override;

        private:
            void onKeyUp(int);
            void onKeyDown(int);
            mpc::Mpc &mpc;
            void deleteDisclaimer();
            std::string name = "default_compact";
            std::vector<juce::Component*> components;
            std::vector<juce::MouseListener*> mouseListeners;
            node view_root;
            const std::function<float()> getScale;
            const std::function<juce::Font&()> getNimbusSansScaled;
            const std::function<juce::Font&()> getMpc2000xlFaceplateGlyphsScaled;
            Keyboard *keyboard = nullptr;
            LedController *ledController = nullptr;
            TooltipOverlay *tooltipOverlay = nullptr;
            Menu *menu = nullptr;
            Disclaimer *disclaimer = nullptr;
            About *about = nullptr;

            friend class Lcd;
    };

} // namespace vmpc_juce::gui::vector
