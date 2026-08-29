#include "gui/mobile/MobileFileWorkflow.hpp"
#include "gui/mobile/MobileFileWorkflowUtil.hpp"

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
        "*.wav;*.WAV;*.snd;*.SND;*.aps;*.APS;*.pgm;*.PGM;*.all;*.ALL;*.mid;*."
        "MID";

    juce::String mimeTypeForFile(const juce::File &file)
    {
        const auto extension = file.getFileExtension().toLowerCase();
        if (extension == ".wav")
        {
            return "audio/wav";
        }
        if (extension == ".mid")
        {
            return "audio/midi";
        }
        if (extension == ".zip")
        {
            return "application/zip";
        }
        return "application/octet-stream";
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

    const auto shareRoot =
        juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getChildFile("VMPC2000XL-shares");
    const auto oldest =
        juce::Time::getCurrentTime() - juce::RelativeTime::days(7);
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

MobileMenuActions
MobileFileWorkflow::makeMenuActions(std::function<void()> togglePhoneFullscreen)
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

void MobileFileWorkflow::defer(std::function<void()> deferredAction)
{
    const std::weak_ptr<int> weakLifetime(lifetime);
    juce::MessageManager::callAsync(
        [weakLifetime, callback = std::move(deferredAction)]
        {
            if (!weakLifetime.expired())
            {
                callback();
            }
        });
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
                               defer(
                                   [this]
                                   {
                                       importAndroidFiles();
                                   });
                           }
                           else if (result == 2)
                           {
                               defer(
                                   [this]
                                   {
                                       importAndroidDirectory();
                                   });
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
    fileChooser->launchAsync(
        flags,
        [this, weakLifetime](const auto &chooser)
        {
            if (weakLifetime.expired())
            {
                return;
            }
            const auto urls = chooser.getURLResults();
            defer(
                [this, urls]
                {
                    fileChooser.reset();
                    if (urls.isEmpty())
                    {
                        return;
                    }
                    pendingImports.clear();
                    for (const auto &url : urls)
                    {
                        collectAndroidDocument(
                            juce::AndroidDocument::fromDocument(url), {});
                    }
                    beginAndroidImport();
                });
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
    fileChooser->launchAsync(
        flags,
        [this, weakLifetime](const auto &chooser)
        {
            if (weakLifetime.expired())
            {
                return;
            }
            const auto urls = chooser.getURLResults();
            defer(
                [this, urls]
                {
                    fileChooser.reset();
                    if (urls.isEmpty())
                    {
                        return;
                    }
                    pendingImports.clear();
                    collectAndroidDocument(
                        juce::AndroidDocument::fromTree(urls.getFirst()), {});
                    beginAndroidImport();
                });
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
        if (info.canRead() && hasSupportedMpcFileExtension(info.getName()))
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
            defer(
                [this, result]
                {
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
#if JUCE_ANDROID
    menu.addItem(1, "Save current project to Files...");

    const auto selected = mpc.screens->get<mpc::lcdgui::ScreenId::LoadScreen>()
                              ->getSelectedFile();
    menu.addItem(2, "Save selected file or folder...", selected != nullptr);
    menu.addSeparator();
    menu.addItem(3, "Share current project...");
    menu.addItem(4, "Share selected file or folder...", selected != nullptr);
    menu.addSeparator();
    menu.addItem(5, "Recording Manager");
#else
    menu.addItem(1, "Share current project");

    const auto selected = mpc.screens->get<mpc::lcdgui::ScreenId::LoadScreen>()
                              ->getSelectedFile();
    menu.addItem(2, "Share selected file or directory", selected != nullptr);
    menu.addItem(3, "Open Recording Manager");
#endif
    const std::weak_ptr<int> weakLifetime(lifetime);
    menu.showMenuAsync(juce::PopupMenu::Options().withParentComponent(&parent),
                       [this, weakLifetime](const int result)
                       {
                           if (weakLifetime.expired())
                           {
                               return;
                           }
#if JUCE_ANDROID
                           if (result == 1)
                           {
                               defer(
                                   [this]
                                   {
                                       saveCurrentProject();
                                   });
                           }
                           else if (result == 2)
                           {
                               defer(
                                   [this]
                                   {
                                       saveSelectedFileOrDirectory();
                                   });
                           }
                           else if (result == 3)
                           {
                               defer(
                                   [this]
                                   {
                                       shareCurrentProject();
                                   });
                           }
                           else if (result == 4)
                           {
                               defer(
                                   [this]
                                   {
                                       shareSelectedFileOrDirectory();
                                   });
                           }
                           else if (result == 5)
                           {
                               defer(
                                   [this]
                                   {
                                       showRecordingManager();
                                   });
                           }
#else
                           if (result == 1)
                           {
                               defer([this]
                                     {
                                         shareCurrentProject();
                                     });
                           }
                           else if (result == 2)
                           {
                               defer([this]
                                     {
                                         shareSelectedFileOrDirectory();
                                     });
                           }
                           else if (result == 3)
                           {
                               defer([this]
                                     {
                                         showRecordingManager();
                                     });
                           }
#endif
                       });
}

juce::File
MobileFileWorkflow::prepareCurrentProject(const juce::String &failureTitle)
{
    auto directory = createShareDirectory("project");

    const auto aps = directory.getChildFile("ALL_PGMS.APS");
    const auto all = directory.getChildFile("ALL_SEQS.ALL");
    if (!writeBytes(aps, mpc::file::kaitai::ApsIo::save(mpc, "ALL_PGMS")) ||
        !writeBytes(all, mpc::file::kaitai::AllIo::save(mpc)))
    {
        directory.deleteRecursively();
        showError(failureTitle, "The project files could not be created.");
        return {};
    }

    for (const auto &sound : mpc.getSampler()->getSounds())
    {
        const auto filename =
            juce::File::createLegalFileName(
                juce::String(sound->getName()).toUpperCase()) +
            ".SND";
        const auto file = directory.getChildFile(filename);
        if (!writeBytes(file, mpc::file::kaitai::SndIo::saveSound(*sound)))
        {
            directory.deleteRecursively();
            showError(failureTitle, "A sound file could not be created.");
            return {};
        }
    }
    return directory;
}

void MobileFileWorkflow::saveCurrentProject()
{
#if JUCE_ANDROID
    const auto directory = prepareCurrentProject("Export failed");
    if (directory == juce::File{})
    {
        return;
    }
    saveDirectory(directory,
                  "VMPC2000XL Project " +
                      juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S"));
#endif
}

void MobileFileWorkflow::saveSelectedFileOrDirectory()
{
#if JUCE_ANDROID
    const auto selected = mpc.screens->get<mpc::lcdgui::ScreenId::LoadScreen>()
                              ->getSelectedFile();
    if (selected == nullptr)
    {
        showError("Nothing selected", "Select a file or directory first.");
        return;
    }

    const auto selectedFile = asJuceFile(selected->getPath());
    if (selected->isDirectory())
    {
        saveDirectory(selectedFile, selectedFile.getFileName());
    }
    else
    {
        saveFile(selectedFile);
    }
#endif
}

void MobileFileWorkflow::saveFile(const juce::File &source)
{
#if JUCE_ANDROID
    fileChooser = std::make_unique<juce::FileChooser>(
        "Save " + source.getFileName(), source, "*" + source.getFileExtension(),
        true, false, &parent);
    const auto flags = juce::FileBrowserComponent::saveMode |
                       juce::FileBrowserComponent::canSelectFiles |
                       juce::FileBrowserComponent::warnAboutOverwriting;
    const std::weak_ptr<int> weakLifetime(lifetime);
    fileChooser->launchAsync(
        flags,
        [this, weakLifetime, source](const auto &chooser)
        {
            if (weakLifetime.expired())
            {
                return;
            }
            const auto urls = chooser.getURLResults();
            defer(
                [this, source, urls]
                {
                    fileChooser.reset();
                    if (urls.isEmpty())
                    {
                        return;
                    }
                    auto destination =
                        juce::AndroidDocument::fromDocument(urls.getFirst());
                    if (!destination.hasValue() ||
                        !copyFileToDocument(source, destination))
                    {
                        showError("Export failed",
                                  "The selected file could "
                                  "not be saved.");
                        return;
                    }
                    const auto name = destination.getInfo().getName();
                    showSuccess(
                        "Export complete",
                        "Saved " +
                            (name.isNotEmpty() ? name : source.getFileName()) +
                            ".");
                });
        });
#else
    juce::ignoreUnused(source);
#endif
}

void MobileFileWorkflow::saveDirectory(const juce::File &source,
                                       const juce::String &destinationName)
{
#if JUCE_ANDROID
    fileChooser = std::make_unique<juce::FileChooser>(
        "Choose an export destination", juce::File{}, "*", true, false,
        &parent);
    const auto flags = juce::FileBrowserComponent::openMode |
                       juce::FileBrowserComponent::canSelectDirectories;
    const std::weak_ptr<int> weakLifetime(lifetime);
    fileChooser->launchAsync(
        flags,
        [this, weakLifetime, source, destinationName](const auto &chooser)
        {
            if (weakLifetime.expired())
            {
                return;
            }
            const auto urls = chooser.getURLResults();
            defer(
                [this, source, destinationName, urls]
                {
                    fileChooser.reset();
                    if (urls.isEmpty())
                    {
                        return;
                    }
                    const auto parentDocument =
                        juce::AndroidDocument::fromTree(urls.getFirst());
                    juce::String createdName;
                    const auto destination = createUniqueChildDirectory(
                        parentDocument, destinationName, createdName);
                    if (!destination.hasValue())
                    {
                        showError("Export failed",
                                  "A destination folder "
                                  "could not be created.");
                        return;
                    }

                    int filesCopied = 0;
                    if (!copyDirectoryToDocument(source, destination,
                                                 filesCopied))
                    {
                        destination.deleteDocument();
                        showError("Export failed",
                                  "The folder could not be "
                                  "saved completely.");
                        return;
                    }

                    showSuccess(
                        "Export complete",
                        "Saved " + juce::String(filesCopied) +
                            (filesCopied == 1 ? " file to " : " files to ") +
                            createdName + ".");
                });
        });
#else
    juce::ignoreUnused(source, destinationName);
#endif
}

juce::AndroidDocument MobileFileWorkflow::createUniqueChildDirectory(
    const juce::AndroidDocument &parentDocument,
    const juce::String &requestedName, juce::String &createdName)
{
#if JUCE_ANDROID
    if (!parentDocument.hasValue() ||
        !parentDocument.getInfo().canCreateChildren())
    {
        return {};
    }

    juce::StringArray existingNames;
    for (auto iterator =
             juce::AndroidDocumentIterator::makeNonRecursive(parentDocument);
         iterator != juce::AndroidDocumentIterator{}; ++iterator)
    {
        existingNames.add((*iterator).getInfo().getName());
    }

    createdName = makeUniqueDocumentName(requestedName, existingNames);
    return parentDocument.createChildDirectory(createdName);
#else
    juce::ignoreUnused(parentDocument, requestedName, createdName);
    return {};
#endif
}

bool MobileFileWorkflow::copyDirectoryToDocument(
    const juce::File &source, const juce::AndroidDocument &destination,
    int &filesCopied)
{
#if JUCE_ANDROID
    for (const auto &child :
         source.findChildFiles(juce::File::findFilesAndDirectories, false, "*"))
    {
        if (child.isDirectory())
        {
            const auto childDestination =
                destination.createChildDirectory(child.getFileName());
            if (!childDestination.hasValue() ||
                !copyDirectoryToDocument(child, childDestination, filesCopied))
            {
                return false;
            }
            continue;
        }

        const auto childDestination =
            destination.createChildDocumentWithTypeAndName(
                mimeTypeForFile(child), child.getFileName());
        if (!childDestination.hasValue() ||
            !copyFileToDocument(child, childDestination))
        {
            if (childDestination.hasValue())
            {
                childDestination.deleteDocument();
            }
            return false;
        }
        ++filesCopied;
    }
    return true;
#else
    juce::ignoreUnused(source, destination, filesCopied);
    return false;
#endif
}

bool MobileFileWorkflow::copyFileToDocument(
    const juce::File &source, const juce::AndroidDocument &destination)
{
#if JUCE_ANDROID
    auto input = source.createInputStream();
    auto output = destination.createOutputStream();
    if (input == nullptr || output == nullptr)
    {
        return false;
    }

    const auto sourceSize = source.getSize();
    const auto bytesWritten = output->writeFromInputStream(*input, -1);
    output->flush();
    output.reset();
    if (bytesWritten != sourceSize)
    {
        return false;
    }

    const auto destinationInfo = destination.getInfo();
    return !destinationInfo.isSizeInBytesValid() ||
           destinationInfo.getSizeInBytes() == sourceSize;
#else
    juce::ignoreUnused(source, destination);
    return false;
#endif
}

void MobileFileWorkflow::shareCurrentProject()
{
    const auto directory = prepareCurrentProject("Share failed");
    if (directory == juce::File{})
    {
        return;
    }
    shareFiles(directory.findChildFiles(juce::File::findFiles, false, "*"));
}

void MobileFileWorkflow::shareSelectedFileOrDirectory()
{
    const auto selected = mpc.screens->get<mpc::lcdgui::ScreenId::LoadScreen>()
                              ->getSelectedFile();
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
        showError("Share failed",
                  "The selected directory could not be zipped.");
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

    defer(
        [this, urls]
        {
            shareSession.close();
            const std::weak_ptr<int> weakLifetime(lifetime);
            shareSession = juce::ContentSharer::shareFilesScoped(
                urls,
                [this, weakLifetime](const bool success,
                                     const juce::String &error)
                {
                    juce::MessageManager::callAsync(
                        [this, weakLifetime, success, error]
                        {
                            if (weakLifetime.expired())
                            {
                                return;
                            }
                            shareSession.close();
                            if (!success && error.isNotEmpty())
                            {
                                showError("Share failed", error);
                            }
                        });
                },
                &parent);
        });
}

juce::File MobileFileWorkflow::createShareDirectory(const juce::String &stem)
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

void MobileFileWorkflow::showSuccess(const juce::String &title,
                                     const juce::String &message)
{
    MLOG(title.toStdString() + ": " + message.toStdString());
    juce::NativeMessageBox::showMessageBoxAsync(
        juce::MessageBoxIconType::InfoIcon, title, message, &parent);
}
