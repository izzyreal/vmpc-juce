#pragma once

#include <juce_data_structures/juce_data_structures.h>

using namespace juce;

namespace vmpc_juce::standalone
{
    struct PropertiesFileOptions : public PropertiesFile::Options
    {
        PropertiesFileOptions()
        {
            applicationName = JucePlugin_Name;
            filenameSuffix = ".settings";
            osxLibrarySubFolder = "Application Support";
            folderName = getOptionsFolderName();
        }

        static String getOptionsFolderName()
        {
#if JUCE_LINUX
            return "~/.config";
#else
            return "";
#endif
        }
    };
} // namespace vmpc_juce
