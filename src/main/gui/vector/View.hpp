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

    class View : public juce::Component {

        public:
            View(mpc::Mpc &mpc,
                    const std::function<float()>& getScale,
                    const std::function<juce::Font&()> &getNimbusSansScaled,
                    const std::function<void()> &showAudioSettingsDialog,
                    const std::function<void()> &resetWindowSize);
            ~View() override;

            void resized() override;

        private:
            std::string name = "default_compact";
            std::vector<juce::Component*> components;
            std::vector<juce::MouseListener*> mouseListeners;
            node view_root;
            const std::function<float()> getScale;
            const std::function<juce::Font&()> getNimbusSansScaled;
            Keyboard *keyboard = nullptr;
            LedController *ledController = nullptr;
            TooltipOverlay *tooltipOverlay = nullptr;
            Menu *menu = nullptr;

            friend class Lcd;
    };

} // namespace vmpc_juce::gui::vector
