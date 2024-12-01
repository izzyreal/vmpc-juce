#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

#include "KeyboardButton.hpp"

#if JUCE_IOS
#include "ImportDocumentUrlProcessor.h"
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
    ImportDocumentUrlProcessor importDocumentUrlProcessor;
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
