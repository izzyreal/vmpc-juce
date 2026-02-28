#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE

#include "ImportDocumentUrlProcessor.hpp"

#include "Mpc.hpp"
#include "disk/AbstractDisk.hpp"
#include "disk/MpcFile.hpp"

using namespace vmpc_juce::gui::ios;

std::string ImportDocumentUrlProcessor::destinationDir()
{
    return mpc->getDisk()->getAbsolutePath();
}

bool ImportDocumentUrlProcessor::destinationExists(const char *filename,
                                                   const char *relativePath)
{
    auto newFilePath =
        mpc_fs::path(destinationDir()).append(relativePath).append(filename);
    return mpc_fs::exists(newFilePath);
}

std::shared_ptr<std::ostream>
ImportDocumentUrlProcessor::openOutputStream(const char *filename,
                                             const char *relativePath)
{
    auto newFileDir = mpc_fs::path(destinationDir()).append(relativePath);
    mpc_fs::create_directories(newFileDir);
    auto newFilePath = newFileDir.append(filename);
    mpc::disk::MpcFile newFile(newFilePath);
    return newFile.getOutputStream();
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
