#include <juce_gui_basics/juce_gui_basics.h>

#include "Node.hpp"

#include <functional>

namespace mpc { class Mpc; }

class Keyboard;

namespace vmpc_juce::gui::vector {

    class Lcd;

    class View : public juce::Component {

        public:
            View(mpc::Mpc &mpc, const std::function<float()>& getScale, const std::function<juce::Font&()> &getNimbusSansScaled);
            ~View() override;

            void resized() override;

        private:
            std::string name = "default_compact";
            std::vector<juce::Component*> components;
            node view_root;
            const std::function<float()> getScale;
            const std::function<juce::Font&()> getNimbusSansScaled;
            Keyboard* keyboard = nullptr;

            friend class Lcd;
    };

} // namespace vmpc_juce::gui::vector
