#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

#include "KeyboardButton.hpp"

#if JUCE_IOS
#include "ImportDocumentUrlProcessor.h"
#endif

namespace mpc { class Mpc; }

class TopRightMenu : public juce::Component {

private:
    juce::Image helpImg;
    juce::Image gearImg;
    juce::Image keyboardImg;
    juce::ImageButton helpButton;
    juce::ImageButton gearButton;
    KeyboardButton keyboardButton;

#if JUCE_IOS
  ImportDocumentUrlProcessor importDocumentUrlProcessor;
    juce::Image importImg;
    juce::Image exportImg;
    juce::ImageButton importButton;
    juce::ImageButton exportButton;
#else
    juce::Image resetWindowSizeImg;
    juce::ImageButton resetWindowSizeButton;
#endif

public:
    TopRightMenu(mpc::Mpc&, std::function<void()>& showAudioSettingsDialog);
    
    void resized() override;
};
