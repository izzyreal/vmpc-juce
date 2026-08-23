#include "gui/mobile/RecordingManager.hpp"

#include <algorithm>

using namespace vmpc_juce::gui::mobile;

namespace
{
    juce::String formattedSize(const juce::File &directory)
    {
        juce::int64 bytes = 0;
        for (const auto &file : directory.findChildFiles(
                 juce::File::findFiles, true))
        {
            bytes += file.getSize();
        }

        return juce::File::descriptionOfSizeInBytes(bytes);
    }
} // namespace

RecordingManager::RecordingManager(
    juce::File recordingsDirectoryToUse, StartPreview startPreviewToUse,
    std::function<void()> stopPreviewToUse,
    std::function<bool()> isPreviewPlayingToUse,
    std::function<void(const juce::File &)> shareRecordingToUse,
    std::function<void()> closeToUse)
    : recordingsDirectory(std::move(recordingsDirectoryToUse)),
      startPreview(std::move(startPreviewToUse)),
      stopPreview(std::move(stopPreviewToUse)),
      isPreviewPlaying(std::move(isPreviewPlayingToUse)),
      shareRecording(std::move(shareRecordingToUse)),
      close(std::move(closeToUse))
{
    title.setText("Recording Manager", juce::dontSendNotification);
    title.setJustificationType(juce::Justification::centred);
    title.setFont(juce::Font(22.f, juce::Font::bold));
    addAndMakeVisible(title);
    addAndMakeVisible(list);

    for (auto *button : {&play, &stop, &share, &remove, &done})
    {
        button->addListener(this);
        addAndMakeVisible(button);
    }

    list.setRowHeight(44);
    list.setColour(juce::ListBox::backgroundColourId,
                   juce::Colours::transparentBlack);
    refresh();
    startTimer(200);
}

RecordingManager::~RecordingManager()
{
    stopPreview();
    for (auto *button : {&play, &stop, &share, &remove, &done})
    {
        button->removeListener(this);
    }
}

void RecordingManager::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colours::black.withAlpha(0.72f));
    const auto panel = getLocalBounds().reduced(juce::jmax(12, getWidth() / 20))
                           .toFloat();
    g.setColour(juce::Colour(0xfff2f5f1));
    g.fillRoundedRectangle(panel, 12.f);
    g.setColour(juce::Colours::black.withAlpha(0.25f));
    g.drawRoundedRectangle(panel, 12.f, 1.f);
}

void RecordingManager::resized()
{
    auto area = getLocalBounds().reduced(juce::jmax(20, getWidth() / 16));
    title.setBounds(area.removeFromTop(48));
    auto controls = area.removeFromBottom(48);
    controls.removeFromTop(6);
    const auto buttonWidth = juce::jmax(58, controls.getWidth() / 5);
    for (auto *button : {&play, &stop, &share, &remove})
    {
        button->setBounds(controls.removeFromLeft(buttonWidth).reduced(3));
    }
    done.setBounds(controls.reduced(3));
    list.setBounds(area.reduced(2));
}

int RecordingManager::getNumRows()
{
    return recordings.size();
}

void RecordingManager::paintListBoxItem(const int row, juce::Graphics &g,
                                        const int width, const int height,
                                        const bool selected)
{
    if (!juce::isPositiveAndBelow(row, recordings.size()))
    {
        return;
    }

    if (selected)
    {
        g.fillAll(juce::Colour(0xffd8e7df));
    }
    g.setColour(juce::Colours::black);
    auto bounds = juce::Rectangle<int>(0, 0, width, height).reduced(10, 2);
    g.drawText(recordings[row].getFileName(), bounds.removeFromLeft(
                   juce::jmax(0, bounds.getWidth() - 120)),
               juce::Justification::centredLeft, true);
    g.setColour(juce::Colours::darkgrey);
    g.drawText(formattedSize(recordings[row]), bounds,
               juce::Justification::centredRight, true);
}

void RecordingManager::selectedRowsChanged(int)
{
    stopPreview();
    updateButtons();
}

void RecordingManager::buttonClicked(juce::Button *button)
{
    if (button == &done)
    {
        close();
        return;
    }
    if (button == &stop)
    {
        stopPreview();
        return;
    }

    const auto directory = selectedDirectory();
    if (directory == juce::File{})
    {
        return;
    }
    if (button == &play)
    {
        if (!startPreview(selectedPreviewFile()))
        {
            juce::NativeMessageBox::showMessageBoxAsync(
                juce::MessageBoxIconType::WarningIcon, "Playback failed",
                "This recording has no readable audio preview.", this);
        }
    }
    else if (button == &share)
    {
        stopPreview();
        shareRecording(directory);
    }
    else if (button == &remove)
    {
        const auto name = directory.getFileName();
        const auto options =
            juce::MessageBoxOptions()
                .withIconType(juce::MessageBoxIconType::WarningIcon)
                .withTitle("Delete recording?")
                .withMessage("Delete \"" + name +
                             "\" and all files inside it?")
                .withButton("Delete")
                .withButton("Cancel")
                .withAssociatedComponent(this);
        deleteConfirmation = juce::NativeMessageBox::showScopedAsync(
            options,
            [safeThis = juce::Component::SafePointer(this), directory](
                const int result)
                {
                    if (safeThis == nullptr || result != 1)
                    {
                        return;
                    }
                    safeThis->stopPreview();
                    if (!directory.deleteRecursively())
                    {
                        juce::NativeMessageBox::showMessageBoxAsync(
                            juce::MessageBoxIconType::WarningIcon,
                            "Delete failed",
                            "The recording could not be deleted.", safeThis);
                    }
                    safeThis->refresh();
                });
    }
}

void RecordingManager::refresh()
{
    recordings = recordingsDirectory.findChildFiles(
        juce::File::findDirectories, false);
    std::sort(recordings.begin(), recordings.end(),
              [](const juce::File &lhs, const juce::File &rhs)
              {
                  return lhs.getFileName().compareIgnoreCase(
                             rhs.getFileName()) < 0;
              });
    list.updateContent();
    if (!recordings.isEmpty())
    {
        list.selectRow(0);
    }
    updateButtons();
}

juce::File RecordingManager::selectedDirectory() const
{
    const auto row = list.getSelectedRow();
    return juce::isPositiveAndBelow(row, recordings.size()) ? recordings[row]
                                                            : juce::File{};
}

juce::File RecordingManager::selectedPreviewFile() const
{
    const auto directory = selectedDirectory();
    auto preview = directory.getChildFile("L.wav");
    if (!preview.existsAsFile())
    {
        preview = directory.getChildFile("L-R.wav");
    }
    if (!preview.existsAsFile())
    {
        const auto wavs = directory.findChildFiles(juce::File::findFiles,
                                                   false, "*.wav;*.WAV");
        preview = wavs.isEmpty() ? juce::File{} : wavs.getFirst();
    }
    return preview;
}

void RecordingManager::updateButtons()
{
    const auto hasSelection = selectedDirectory() != juce::File{};
    play.setEnabled(hasSelection);
    stop.setEnabled(hasSelection && isPreviewPlaying());
    share.setEnabled(hasSelection);
    remove.setEnabled(hasSelection);
}

void RecordingManager::timerCallback()
{
    updateButtons();
}
