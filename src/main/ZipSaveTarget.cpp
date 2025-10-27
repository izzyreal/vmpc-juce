#include "ZipSaveTarget.hpp"

using namespace vmpc_juce;

ZipSaveTarget::ZipSaveTarget(const void* data, size_t size)
{
    juce::MemoryInputStream input(data, size, false);
    juce::ZipFile zip(input);

    for (int i = 0; i < zip.getNumEntries(); ++i)
    {
        auto* entry = zip.getEntry(i);
        if (!entry) continue;

        std::unique_ptr<juce::InputStream> stream(zip.createStreamForEntry(i));
        if (!stream) continue;

        juce::MemoryOutputStream memOut;
        memOut.writeFromInputStream(*stream, -1);
        const auto bytes = memOut.getMemoryBlock();
        const auto filename = entry->filename.toStdString();

        files[filename] = std::vector<char>((const char*)bytes.getData(),
                                            (const char*)bytes.getData() + bytes.getSize());
    }
}

void ZipSaveTarget::setFileData(const fs::path& path, const std::vector<char>& data)
{
    const auto key = path.string();
    if (data.empty())
        files.erase(key);
    else
        files[key] = std::vector<char>(data.begin(), data.end());
}

std::vector<char> ZipSaveTarget::getFileData(const fs::path& path) const
{
    const auto key = path.string();
    auto it = files.find(key);
    if (it == files.end()) return {};
    return it->second;
}

bool ZipSaveTarget::exists(const fs::path& path) const
{
    return files.find(path.string()) != files.end();
}

std::uintmax_t ZipSaveTarget::fileSize(const fs::path& path) const
{
    auto it = files.find(path.string());
    if (it == files.end()) return 0;
    return it->second.size();
}

std::unique_ptr<juce::MemoryBlock> ZipSaveTarget::toZipMemoryBlock() const
{
    juce::MemoryOutputStream memOut;
    juce::ZipFile::Builder builder;

    for (const auto& [name, data] : files)
    {
        juce::MemoryInputStream inputStream(data.data(), data.size(), false);
        builder.addEntry(&inputStream,
                         0,
                         name,
                         juce::Time::getCurrentTime());
    }

    builder.writeToStream(memOut, nullptr);
    return std::make_unique<juce::MemoryBlock>(memOut.getData(), memOut.getDataSize());
}
