/*
    This file is part of vmpc-juce, a JUCE implementation of VMPC2000XL.

    vmpc-juce is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License (GPL) as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    vmpc-juce is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with vmpc-juce. If not, see <https://www.gnu.org/licenses/>.

    This project uses JUCE, which is licensed under the GNU Affero General Public License (AGPL).
    See <https://juce.com> for details.
*/
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

bool ImportDocumentUrlProcessor::destinationExists(const char* filename, const char* relativePath)
{
  auto newFilePath = fs::path(destinationDir()).append(relativePath).append(filename);
  return fs::exists(newFilePath);
}

std::shared_ptr<std::ostream> ImportDocumentUrlProcessor::openOutputStream(const char* filename, const char* relativePath)
{
  auto newFileDir = fs::path(destinationDir()).append(relativePath);
  fs::create_directories(newFileDir);
  auto newFilePath = newFileDir.append(filename);
  mpc::disk::MpcFile newFile(newFilePath);
  return newFile.getOutputStream();
}

void ImportDocumentUrlProcessor::initFiles()
{
    auto layeredScreen = mpc->getLayeredScreen();
    auto currentScreen = layeredScreen->getCurrentScreenName();
    if (currentScreen == "load" || currentScreen == "save" || currentScreen == "directory")
    {
        layeredScreen->openScreen(currentScreen == "directory" ? "load" : "black");
        mpc->getDisk()->initFiles();
        layeredScreen->openScreen(currentScreen);
    }
}

#endif
#endif
