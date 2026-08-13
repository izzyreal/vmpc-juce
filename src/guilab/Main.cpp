#include "GuiLabComponent.hpp"

#include <juce_gui_extra/juce_gui_extra.h>

namespace vmpc_juce::guilab
{
    class GuiLabApplication final : public juce::JUCEApplication
    {
    public:
        const juce::String getApplicationName() override
        {
            return "VMPC2000XL GUI Lab";
        }

        const juce::String getApplicationVersion() override
        {
            return JUCE_APPLICATION_VERSION_STRING;
        }

        bool moreThanOneInstanceAllowed() override
        {
            return true;
        }

        void initialise(const juce::String &) override
        {
            mainWindow = std::make_unique<MainWindow>(getApplicationName());
        }

        void shutdown() override
        {
            mainWindow.reset();
        }

        void systemRequestedQuit() override
        {
            quit();
        }

        void anotherInstanceStarted(const juce::String &) override {}

    private:
        class MainWindow final : public juce::DocumentWindow
        {
        public:
            explicit MainWindow(const juce::String &name)
                : DocumentWindow(name, juce::Colour(0xff202523),
                                 DocumentWindow::allButtons)
            {
                setUsingNativeTitleBar(true);
                setContentOwned(new GuiLabComponent(), true);
                setResizable(true, false);
                setResizeLimits(760, 540, 4096, 4096);
                centreWithSize(1280, 900);
                setVisible(true);
            }

            void closeButtonPressed() override
            {
                if (auto *app = JUCEApplication::getInstance())
                {
                    app->systemRequestedQuit();
                }
            }
        };

        std::unique_ptr<MainWindow> mainWindow;
    };
} // namespace vmpc_juce::guilab

START_JUCE_APPLICATION(vmpc_juce::guilab::GuiLabApplication)
