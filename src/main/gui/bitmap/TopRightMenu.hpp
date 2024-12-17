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

#include <functional>

#include "KeyboardButton.hpp"

#if JUCE_IOS
#include "gui/ios/ImportDocumentUrlProcessor.hpp"
#endif

namespace mpc { class Mpc; }

namespace vmpc_juce::gui::bitmap {

class TopRightMenu : public juce::Component {

private:
    juce::Image helpImg;
    juce::ImageButton helpButton;
    juce::Image gearImg;
    juce::ImageButton gearButton;
    juce::Image keyboardImg;
    KeyboardButton keyboardButton;

#if JUCE_IOS
    vmpc_juce::gui::ios::ImportDocumentUrlProcessor importDocumentUrlProcessor;
    juce::Image importImg;
    juce::ImageButton importButton;
    juce::Image exportImg;
    juce::ImageButton exportButton;
    juce::Image recordingManagerImg;
    juce::ImageButton recordingManagerButton;
#else
    juce::Image resetWindowSizeImg;
    juce::ImageButton resetWindowSizeButton;
#endif

public:
    TopRightMenu(mpc::Mpc&, std::function<void()>& showAudioSettingsDialog);
    
    void resized() override;
};

} // namespace vmpc_juce::gui::bitmap
