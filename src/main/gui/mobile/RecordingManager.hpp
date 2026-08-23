#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace vmpc_juce::gui::mobile
{
    class RecordingManager final : public juce::Component,
                                   private juce::ListBoxModel,
                                   private juce::Button::Listener,
                                   private juce::Timer
    {
    public:
        using StartPreview = std::function<bool(const juce::File &)>;

        RecordingManager(juce::File recordingsDirectory,
                         StartPreview startPreview,
                         std::function<void()> stopPreview,
                         std::function<bool()> isPreviewPlaying,
                         std::function<void(const juce::File &)> shareRecording,
                         std::function<void()> close);
        ~RecordingManager() override;

        void paint(juce::Graphics &) override;
        void resized() override;

    private:
        int getNumRows() override;
        void paintListBoxItem(int row, juce::Graphics &, int width, int height,
                              bool selected) override;
        void selectedRowsChanged(int row) override;
        void buttonClicked(juce::Button *) override;
        void refresh();
        juce::File selectedDirectory() const;
        juce::File selectedPreviewFile() const;
        void updateButtons();
        void timerCallback() override;

        juce::File recordingsDirectory;
        StartPreview startPreview;
        std::function<void()> stopPreview;
        std::function<bool()> isPreviewPlaying;
        std::function<void(const juce::File &)> shareRecording;
        std::function<void()> close;
        juce::Array<juce::File> recordings;

        juce::Label title;
        juce::ListBox list{"Recordings", this};
        juce::TextButton play{"Play"};
        juce::TextButton stop{"Stop"};
        juce::TextButton share{"Share"};
        juce::TextButton remove{"Delete"};
        juce::TextButton done{"Done"};
        juce::ScopedMessageBox deleteConfirmation;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecordingManager)
    };
} // namespace vmpc_juce::gui::mobile
