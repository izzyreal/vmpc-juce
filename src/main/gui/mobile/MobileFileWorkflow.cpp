#include "gui/mobile/MobileFileWorkflow.hpp"

#include "gui/mobile/RecordingManager.hpp"
#include "gui/ios/ImportDocumentUrlProcessor.hpp"

#include "Logger.hpp"
#include "Mpc.hpp"
#include "disk/AbstractDisk.hpp"
#include "disk/MpcFile.hpp"
#include "file/kaitai/AllIo.hpp"
#include "file/kaitai/ApsIo.hpp"
#include "file/kaitai/SndIo.hpp"
#include "lcdgui/screens/LoadScreen.hpp"
#include "sampler/Sampler.hpp"

#include <algorithm>

#if JUCE_IOS
void doOpenIosImportDocumentBrowser(
    vmpc_juce::gui::ios::ImportDocumentUrlProcessor *,
    void *nativeWindowHandle);
#endif

using namespace vmpc_juce::gui::mobile;

namespace
{
#if JUCE_ANDROID
    constexpr auto supportedPatterns =
        "*.wav;*.WAV;*.snd;*.SND;*.aps;*.APS;*.pgm;*.PGM;*.all;*.ALL;*.mid;*.MID";

    bool hasSupportedExtension(const juce::String &name)
    {
        static const juce::StringArray extensions{
            ".wav", ".snd", ".aps", ".pgm", ".all", ".mid"};
        const auto lower = name.toLowerCase();
        return std::any_of(extensions.begin(), extensions.end(),
                           [&](const auto &extension)
                           {
                               return lower.endsWith(extension);
                           });
    }
#endif

    juce::File asJuceFile(const mpc_fs::path &path)
    {
        return juce::File(juce::String::fromUTF8(path.string().c_str()));
    }

    template <typename Container>
    bool writeBytes(const juce::File &file, const Container &bytes)
    {
        return file.replaceWithData(bytes.data(), bytes.size());
    }
} // namespace

MobileFileWorkflow::MobileFileWorkflow(
    mpc::Mpc &mpcToUse, juce::Component &parentToUse,
    StartPreview startPreviewToUse, std::function<void()> stopPreviewToUse,
    std::function<bool()> isPreviewPlayingToUse)
    : mpc(mpcToUse), parent(parentToUse),
      startPreview(std::move(startPreviewToUse)),
      stopPreview(std::move(stopPreviewToUse)),
      isPreviewPlaying(std::move(isPreviewPlayingToUse))
{
#if JUCE_IOS
    iosImportProcessor =
        std::make_unique<gui::ios::ImportDocumentUrlProcessor>();
    iosImportProcessor->mpc = &mpc;
#endif

    const auto shareRoot = juce::File::getSpecialLocation(
                               juce::File::tempDirectory)
                               .getChildFile("VMPC2000XL-shares");
    const auto oldest = juce::Time::getCurrentTime() - juce::RelativeTime::days(7);
    for (const auto &entry :
         shareRoot.findChildFiles(juce::File::findDirectories, false))
    {
        if (entry.getLastModificationTime() < oldest)
        {
            entry.deleteRecursively();
        }
    }
}

MobileFileWorkflow::~MobileFileWorkflow()
{
    lifetime.reset();
    closeRecordingManager();
    fileChooser.reset();
    shareSession.close();
}

MobileMenuActions MobileFileWorkflow::makeMenuActions(
    std::function<void()> togglePhoneFullscreen)
{
    MobileMenuActions result;
    result.importFiles = [this]
    {
        importFiles();
    };
    result.exportFiles = [this]
    {
        showExportOptions();
    };
    result.openRecordingManager = [this]
    {
        showRecordingManager();
    };
    result.togglePhoneFullscreen = std::move(togglePhoneFullscreen);
    return result;
}

void MobileFileWorkflow::resized()
{
    if (recordingManager != nullptr)
    {
        recordingManager->setBounds(parent.getLocalBounds());
    }
}

void MobileFileWorkflow::importFiles()
{
#if JUCE_IOS
    if (auto *peer = parent.getPeer())
    {
        doOpenIosImportDocumentBrowser(iosImportProcessor.get(),
                                       peer->getNativeHandle());
    }
#elif JUCE_ANDROID
    juce::PopupMenu menu;
    menu.addItem(1, "Import files...");
    menu.addItem(2, "Import a folder...");
    const std::weak_ptr<int> weakLifetime(lifetime);
    menu.showMenuAsync(juce::PopupMenu::Options().withParentComponent(&parent),
                       [this, weakLifetime](const int result)
                       {
                           if (weakLifetime.expired())
                           {
                               return;
                           }
                           if (result == 1)
                           {
                               importAndroidFiles();
                           }
                           else if (result == 2)
                           {
                               importAndroidDirectory();
                           }
                       });
#endif
}

void MobileFileWorkflow::importAndroidFiles()
{
#if JUCE_ANDROID
    fileChooser = std::make_unique<juce::FileChooser>(
        "Import MPC files", juce::File{}, supportedPatterns, true, false,
        &parent);
    const auto flags = juce::FileBrowserComponent::openMode |
                       juce::FileBrowserComponent::canSelectFiles |
                       juce::FileBrowserComponent::canSelectMultipleItems;
    const std::weak_ptr<int> weakLifetime(lifetime);
    fileChooser->launchAsync(flags, [this, weakLifetime](const auto &chooser)
                             {
                                 if (weakLifetime.expired())
                                 {
                                     return;
                                 }
                                 pendingImports.clear();
                                 for (const auto &url : chooser.getURLResults())
                                 {
                                     collectAndroidDocument(
                                         juce::AndroidDocument::fromDocument(
                                             url),
                                         {});
                                 }
                                 beginAndroidImport();
                             });
#endif
}

void MobileFileWorkflow::importAndroidDirectory()
{
#if JUCE_ANDROID
    fileChooser = std::make_unique<juce::FileChooser>(
        "Import an MPC folder", juce::File{}, "*", true, false, &parent);
    const auto flags = juce::FileBrowserComponent::openMode |
                       juce::FileBrowserComponent::canSelectDirectories;
    const std::weak_ptr<int> weakLifetime(lifetime);
    fileChooser->launchAsync(flags, [this, weakLifetime](const auto &chooser)
                             {
                                 if (weakLifetime.expired() ||
                                     chooser.getURLResults().isEmpty())
                                 {
                                     return;
                                 }
                                 pendingImports.clear();
                                 collectAndroidDocument(
                                     juce::AndroidDocument::fromTree(
                                         chooser.getURLResult()),
                                     {});
                                 beginAndroidImport();
                             });
#endif
}

void MobileFileWorkflow::collectAndroidDocument(
    const juce::AndroidDocument &document,
    const juce::String &relativeDirectory)
{
#if JUCE_ANDROID
    if (!document.hasValue())
    {
        return;
    }
    const auto info = document.getInfo();
    if (info.isFile())
    {
        if (info.canRead() && hasSupportedExtension(info.getName()))
        {
            pendingImports.push_back(
                {document, relativeDirectory, info.getName()});
        }
        return;
    }
    if (!info.isDirectory())
    {
        return;
    }

    for (auto iterator =
             juce::AndroidDocumentIterator::makeNonRecursive(document);
         iterator != juce::AndroidDocumentIterator{}; ++iterator)
    {
        const auto child = *iterator;
        const auto childInfo = child.getInfo();
        const auto childName =
            juce::File::createLegalFileName(childInfo.getName());
        const auto childRelativeDirectory =
            childInfo.isDirectory()
                ? (relativeDirectory.isEmpty()
                       ? childName
                       : relativeDirectory + "/" + childName)
                : relativeDirectory;
        collectAndroidDocument(child, childRelativeDirectory);
    }
#else
    juce::ignoreUnused(document, relativeDirectory);
#endif
}

void MobileFileWorkflow::beginAndroidImport()
{
    fileChooser.reset();
    nextImport = 0;
    importedCount = skippedCount = failedCount = 0;
    existingFilePolicy = ExistingFilePolicy::ask;
    if (pendingImports.empty())
    {
        showError("Nothing to import",
                  "No supported MPC files were found in the selection.");
        return;
    }
    processNextAndroidImport();
}

void MobileFileWorkflow::processNextAndroidImport()
{
#if JUCE_ANDROID
    if (nextImport >= pendingImports.size())
    {
        finishAndroidImport();
        return;
    }

    const auto &item = pendingImports[nextImport];
    const auto destination =
        asJuceFile(mpc_fs::path(mpc.getDisk()->getAbsolutePath()))
            .getChildFile(item.relativeDirectory)
            .getChildFile(juce::File::createLegalFileName(item.name));

    if (!destination.exists() ||
        existingFilePolicy == ExistingFilePolicy::replaceAll)
    {
        copyCurrentAndroidImport();
        return;
    }
    if (existingFilePolicy == ExistingFilePolicy::skipAll)
    {
        ++skippedCount;
        ++nextImport;
        processNextAndroidImport();
        return;
    }

    juce::PopupMenu overwriteMenu;
    overwriteMenu.addItem(1, "Replace " + destination.getFileName());
    overwriteMenu.addItem(2, "Skip this file");
    overwriteMenu.addSeparator();
    overwriteMenu.addItem(3, "Replace all existing files");
    overwriteMenu.addItem(4, "Skip all existing files");
    const std::weak_ptr<int> weakLifetime(lifetime);
    overwriteMenu.showMenuAsync(
        juce::PopupMenu::Options().withParentComponent(&parent),
        [this, weakLifetime](const int result)
        {
            if (weakLifetime.expired())
            {
                return;
            }
            if (result == 1 || result == 3)
            {
                if (result == 3)
                {
                    existingFilePolicy = ExistingFilePolicy::replaceAll;
                }
                copyCurrentAndroidImport();
            }
            else
            {
                if (result == 4)
                {
                    existingFilePolicy = ExistingFilePolicy::skipAll;
                }
                ++skippedCount;
                ++nextImport;
                processNextAndroidImport();
            }
        });
#endif
}

void MobileFileWorkflow::copyCurrentAndroidImport()
{
#if JUCE_ANDROID
    const auto &item = pendingImports[nextImport];
    auto destinationDirectory =
        asJuceFile(mpc_fs::path(mpc.getDisk()->getAbsolutePath()))
            .getChildFile(item.relativeDirectory);
    const auto destination = destinationDirectory.getChildFile(
        juce::File::createLegalFileName(item.name));

    bool success = destinationDirectory.createDirectory().wasOk();
    if (success && destination.exists())
    {
        success = destination.deleteFile();
    }

    if (success)
    {
        auto input = item.document.createInputStream();
        auto output = destination.createOutputStream();
        success = input != nullptr && output != nullptr;
        if (success)
        {
            const auto bytesWritten = output->writeFromInputStream(*input, -1);
            output->flush();
            const auto info = item.document.getInfo();
            success = output->getStatus().wasOk() &&
                      (!info.isSizeInBytesValid() ||
                       bytesWritten == info.getSizeInBytes());
        }
    }

    if (!success)
    {
        destination.deleteFile();
    }

    success ? ++importedCount : ++failedCount;
    ++nextImport;
    processNextAndroidImport();
#endif
}

void MobileFileWorkflow::finishAndroidImport()
{
    refreshDiskFileList();
    const auto message = juce::String(importedCount) + " imported, " +
                         juce::String(skippedCount) + " skipped, " +
                         juce::String(failedCount) + " failed.";
    juce::NativeMessageBox::showMessageBoxAsync(
        failedCount == 0 ? juce::MessageBoxIconType::InfoIcon
                         : juce::MessageBoxIconType::WarningIcon,
        "Import complete", message, &parent);
    pendingImports.clear();
}

void MobileFileWorkflow::refreshDiskFileList()
{
    const auto layeredScreen = mpc.getLayeredScreen();
    const auto currentScreen = layeredScreen->getCurrentScreenName();
    if (currentScreen == "load" || currentScreen == "save" ||
        currentScreen == "directory")
    {
        layeredScreen->openScreen(currentScreen == "directory" ? "load"
                                                               : "black");
        mpc.getDisk()->initFiles();
        layeredScreen->openScreen(currentScreen);
    }
}

void MobileFileWorkflow::showExportOptions()
{
    juce::PopupMenu menu;
    menu.addItem(1, "Share current project");

    const auto selected =
        mpc.screens->get<mpc::lcdgui::ScreenId::LoadScreen>()->getSelectedFile();
    menu.addItem(2, "Share selected file or directory", selected != nullptr);
    menu.addItem(3, "Open Recording Manager");
    const std::weak_ptr<int> weakLifetime(lifetime);
    menu.showMenuAsync(juce::PopupMenu::Options().withParentComponent(&parent),
                       [this, weakLifetime](const int result)
                       {
                           if (weakLifetime.expired())
                           {
                               return;
                           }
                           if (result == 1)
                           {
                               shareCurrentProject();
                           }
                           else if (result == 2)
                           {
                               shareSelectedFileOrDirectory();
                           }
                           else if (result == 3)
                           {
                               showRecordingManager();
                           }
                       });
}

void MobileFileWorkflow::shareCurrentProject()
{
    auto directory = createShareDirectory("project");
    juce::Array<juce::File> files;

    const auto aps = directory.getChildFile("ALL_PGMS.APS");
    const auto all = directory.getChildFile("ALL_SEQS.ALL");
    if (!writeBytes(aps, mpc::file::kaitai::ApsIo::save(mpc, "ALL_PGMS")) ||
        !writeBytes(all, mpc::file::kaitai::AllIo::save(mpc)))
    {
        showError("Share failed", "The project files could not be created.");
        return;
    }
    files.add(aps);
    files.add(all);

    for (const auto &sound : mpc.getSampler()->getSounds())
    {
        const auto filename = juce::File::createLegalFileName(
                                  juce::String(sound->getName()).toUpperCase()) +
                              ".SND";
        const auto file = directory.getChildFile(filename);
        if (!writeBytes(file, mpc::file::kaitai::SndIo::saveSound(*sound)))
        {
            showError("Share failed", "A sound file could not be created.");
            return;
        }
        files.add(file);
    }
    shareFiles(files);
}

void MobileFileWorkflow::shareSelectedFileOrDirectory()
{
    const auto selected =
        mpc.screens->get<mpc::lcdgui::ScreenId::LoadScreen>()->getSelectedFile();
    if (selected == nullptr)
    {
        showError("Nothing selected", "Select a file or directory first.");
        return;
    }
    const auto selectedFile = asJuceFile(selected->getPath());
    if (!selected->isDirectory())
    {
        shareFiles({selectedFile});
        return;
    }

    const auto shareDirectory = createShareDirectory("selection");
    const auto zip = zipDirectory(
        selectedFile,
        shareDirectory.getChildFile(selectedFile.getFileName() + ".zip"));
    if (zip == juce::File{})
    {
        showError("Share failed", "The selected directory could not be zipped.");
        return;
    }
    shareFiles({zip});
}

void MobileFileWorkflow::shareRecordingDirectory(const juce::File &directory)
{
    const auto shareDirectory = createShareDirectory("recording");
    const auto zip = zipDirectory(
        directory,
        shareDirectory.getChildFile(directory.getFileName() + ".zip"));
    if (zip == juce::File{})
    {
        showError("Share failed", "The recording could not be zipped.");
        return;
    }
    shareFiles({zip});
}

void MobileFileWorkflow::shareFiles(const juce::Array<juce::File> &files)
{
    juce::Array<juce::URL> urls;
    for (const auto &file : files)
    {
        if (file.existsAsFile())
        {
            urls.add(juce::URL(file));
        }
    }
    if (urls.isEmpty())
    {
        showError("Share failed", "No readable files were produced.");
        return;
    }

    shareSession.close();
    const std::weak_ptr<int> weakLifetime(lifetime);
    shareSession = juce::ContentSharer::shareFilesScoped(
        urls,
        [this, weakLifetime](const bool success, const juce::String &error)
        {
            if (!weakLifetime.expired() && !success && error.isNotEmpty())
            {
                showError("Share failed", error);
            }
        },
        &parent);
}

juce::File
MobileFileWorkflow::createShareDirectory(const juce::String &stem)
{
    auto root = juce::File::getSpecialLocation(juce::File::tempDirectory)
                    .getChildFile("VMPC2000XL-shares");
    root.createDirectory();
    auto directory = root.getNonexistentChildFile(
        stem + "-" + juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S"),
        {}, false);
    directory.createDirectory();
    return directory;
}

juce::File MobileFileWorkflow::zipDirectory(const juce::File &directory,
                                            const juce::File &destination)
{
    juce::ZipFile::Builder builder;
    const auto files =
        directory.findChildFiles(juce::File::findFiles, true, "*");
    for (const auto &file : files)
    {
        builder.addFile(file, 9, file.getRelativePathFrom(directory));
    }
    auto output = destination.createOutputStream();
    if (output == nullptr || !builder.writeToStream(*output, nullptr))
    {
        destination.deleteFile();
        return {};
    }
    output->flush();
    return destination;
}

void MobileFileWorkflow::showRecordingManager()
{
    closeRecordingManager();
    const std::weak_ptr<int> weakLifetime(lifetime);
    recordingManager = std::make_unique<RecordingManager>(
        asJuceFile(mpc.paths->getDocuments()->recordingsPath()), startPreview,
        stopPreview, isPreviewPlaying,
        [this, weakLifetime](const juce::File &directory)
        {
            if (!weakLifetime.expired())
            {
                shareRecordingDirectory(directory);
            }
        },
        [this, weakLifetime]
        {
            if (!weakLifetime.expired())
            {
                closeRecordingManager();
            }
        });
    parent.addAndMakeVisible(*recordingManager);
    resized();
    recordingManager->toFront(true);
}

void MobileFileWorkflow::closeRecordingManager()
{
    stopPreview();
    if (recordingManager != nullptr)
    {
        parent.removeChildComponent(recordingManager.get());
        recordingManager.reset();
    }
}

void MobileFileWorkflow::showError(const juce::String &title,
                                   const juce::String &message)
{
    MLOG(title.toStdString() + ": " + message.toStdString());
    juce::NativeMessageBox::showMessageBoxAsync(
        juce::MessageBoxIconType::WarningIcon, title, message, &parent);
}
