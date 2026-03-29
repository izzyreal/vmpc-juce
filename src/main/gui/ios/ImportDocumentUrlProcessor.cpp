#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE

#include "ImportDocumentUrlProcessor.hpp"

#include "FileIoPolicy.hpp"
#include "Mpc.hpp"
#include <Logger.hpp>
#include "disk/AbstractDisk.hpp"
#include "disk/MpcFile.hpp"

using namespace vmpc_juce::gui::ios;
using namespace mpc::file_io;

std::string ImportDocumentUrlProcessor::destinationDir()
{
    return mpc->getDisk()->getAbsolutePath();
}

std::optional<bool>
ImportDocumentUrlProcessor::destinationExists(const char *filename,
                                              const char *relativePath)
{
    auto newFilePath =
        mpc_fs::path(destinationDir()).append(relativePath).append(filename);
    return value(mpc_fs::exists(newFilePath), FailurePolicy::Required,
                 "iOS import destination inspection");
}

std::shared_ptr<std::ostream>
ImportDocumentUrlProcessor::openOutputStream(const char *filename,
                                             const char *relativePath)
{
    auto newFileDir = mpc_fs::path(destinationDir()).append(relativePath);
    if (!success(mpc_fs::create_directories(newFileDir),
                 FailurePolicy::Required, "iOS import destination creation"))
    {
        return {};
    }

    auto newFilePath = newFileDir.append(filename);
    mpc::disk::MpcFile newFile(newFilePath);
    auto outputStream = newFile.getOutputStream();
    if (!outputStream)
    {
        MLOG("Required file I/O failed during iOS import destination open for '" +
             newFilePath.string() + "'");
    }
    return outputStream;
}

void ImportDocumentUrlProcessor::initFiles()
{
    auto layeredScreen = mpc->getLayeredScreen();
    auto currentScreen = layeredScreen->getCurrentScreenName();
    if (currentScreen == "load" || currentScreen == "save" ||
        currentScreen == "directory")
    {
        layeredScreen->openScreen(currentScreen == "directory" ? "load"
                                                               : "black");
        mpc->getDisk()->initFiles();
        layeredScreen->openScreen(currentScreen);
    }
}

#endif
#endif
