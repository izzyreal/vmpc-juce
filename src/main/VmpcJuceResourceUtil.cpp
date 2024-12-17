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
#include "VmpcJuceResourceUtil.hpp"

#ifdef MAC_BUNDLE_RESOURCES
#include "MacBundleResources.h"
#include "mpc_fs.hpp"
#else
#include <cmrc/cmrc.hpp>
#include <string_view>
CMRC_DECLARE(vmpcjuce);
#endif

using namespace vmpc_juce;

std::vector<char> VmpcJuceResourceUtil::getResourceData(const std::string& path)
{
#ifdef MAC_BUNDLE_RESOURCES
    return getResourceDataFromMacBundleResources(path);
#else
    return getResourceDataFromInMemoryFS(path);
#endif
}

juce::Image VmpcJuceResourceUtil::loadImage(const std::string& path)
{
#ifdef MAC_BUNDLE_RESOURCES
    return loadImageFromMacBundleResources(path);
#else
    return loadImageFromInMemoryFS(path);
#endif
}

#ifdef MAC_BUNDLE_RESOURCES
juce::Image VmpcJuceResourceUtil::loadImageFromMacBundleResources(const std::string &path)
{
    const auto imgPath = mpc::MacBundleResources::getResourcePath(path);
    return juce::ImageFileFormat::loadFrom(juce::File(imgPath));
}

std::vector<char> VmpcJuceResourceUtil::getResourceDataFromMacBundleResources(const std::string& path)
{
    const auto resource_path = mpc::MacBundleResources::getResourcePath(path);
    return get_file_data(resource_path);
}
#else

std::vector<char> VmpcJuceResourceUtil::getResourceDataFromInMemoryFS(const std::string& path)
{
    const auto file = cmrc::vmpcjuce::get_filesystem().open(path.c_str());
    const auto data = std::string_view(file.begin(), file.size()).data();
    return { data, data + file.size() };
}

juce::Image VmpcJuceResourceUtil::loadImageFromInMemoryFS(const std::string& path)
{
    const auto file = cmrc::vmpcjuce::get_filesystem().open(path.c_str());
    const auto data = std::string_view(file.begin(), file.size()).data();
    auto stream = juce::MemoryInputStream(data, file.size(), true);
    return juce::ImageFileFormat::loadFrom(stream);
}
#endif
