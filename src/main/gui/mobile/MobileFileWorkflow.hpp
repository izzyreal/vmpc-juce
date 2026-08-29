#pragma once

#include "gui/mobile/MobileMenuActions.hpp"

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>
#include <vector>

namespace mpc
{
    class Mpc;
}

namespace vmpc_juce::gui::ios
{
    class ImportDocumentUrlProcessor;
}

namespace vmpc_juce::gui::mobile
{
    class RecordingManager;

    class MobileFileWorkflow
    {
    public:
        using StartPreview = std::function<bool(const juce::File &)>;

        MobileFileWorkflow(mpc::Mpc &, juce::Component &parent,
                           StartPreview startPreview,
                           std::function<void()> stopPreview,
                           std::function<bool()> isPreviewPlaying);
        ~MobileFileWorkflow();

        MobileMenuActions
        makeMenuActions(std::function<void()> togglePhoneFullscreen);
        void resized();

    private:
        enum class ExistingFilePolicy
        {
            ask,
            replaceAll,
            skipAll
        };

        struct ImportItem
        {
            juce::AndroidDocument document;
            juce::String relativeDirectory;
            juce::String name;
        };

        void importFiles();
        void importAndroidFiles();
        void importAndroidDirectory();
        void defer(std::function<void()>);
        void collectAndroidDocument(const juce::AndroidDocument &,
                                    const juce::String &relativeDirectory);
        void beginAndroidImport();
        void processNextAndroidImport();
        void copyCurrentAndroidImport();
        void finishAndroidImport();
        void refreshDiskFileList();

        void showExportOptions();
        void saveCurrentProject();
        void saveSelectedFileOrDirectory();
        void saveFile(const juce::File &);
        void saveDirectory(const juce::File &,
                           const juce::String &destinationName);
        bool copyDirectoryToDocument(const juce::File &source,
                                     const juce::AndroidDocument &destination,
                                     int &filesCopied);
        bool copyFileToDocument(const juce::File &source,
                                const juce::AndroidDocument &destination);
        juce::AndroidDocument
        createUniqueChildDirectory(const juce::AndroidDocument &parent,
                                   const juce::String &requestedName,
                                   juce::String &createdName);
        juce::File prepareCurrentProject(const juce::String &failureTitle);
        void shareCurrentProject();
        void shareSelectedFileOrDirectory();
        void shareRecordingDirectory(const juce::File &);
        void shareFiles(const juce::Array<juce::File> &);
        juce::File createShareDirectory(const juce::String &stem);
        juce::File zipDirectory(const juce::File &directory,
                                const juce::File &destination);
        void showRecordingManager();
        void closeRecordingManager();
        void showError(const juce::String &title, const juce::String &message);
        void showSuccess(const juce::String &title,
                         const juce::String &message);

        mpc::Mpc &mpc;
        juce::Component &parent;
        StartPreview startPreview;
        std::function<void()> stopPreview;
        std::function<bool()> isPreviewPlaying;
        std::unique_ptr<gui::ios::ImportDocumentUrlProcessor>
            iosImportProcessor;
        std::unique_ptr<juce::FileChooser> fileChooser;
        std::unique_ptr<RecordingManager> recordingManager;
        juce::ScopedMessageBox shareSession;
        std::shared_ptr<int> lifetime = std::make_shared<int>(0);

        std::vector<ImportItem> pendingImports;
        std::size_t nextImport = 0;
        ExistingFilePolicy existingFilePolicy = ExistingFilePolicy::ask;
        int importedCount = 0;
        int skippedCount = 0;
        int failedCount = 0;
    };
} // namespace vmpc_juce::gui::mobile
