/*
    This file is part of vmpc-juce, a JUCE implementation of VMPC2000XL.

    vmpc-juce is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License (GPL) as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    vmpc-juce is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with vmpc-juce. If not, see <https://www.gnu.org/licenses/>.

    This project uses JUCE, which is licensed under the GNU Affero General Public License (AGPL).
    See <https://juce.com> for details.
*/
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
