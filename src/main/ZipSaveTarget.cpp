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

    // Keep the raw data alive while the builder runs.
    // Builder will take ownership of the InputStream* we pass it,
    // but the InputStream must point at memory that remains valid
    // until writeToStream() finishes — hence we own MemoryBlocks here.
    std::vector<std::unique_ptr<juce::MemoryBlock>> ownedBlocks;
    ownedBlocks.reserve(files.size());

    for (const auto& p : files)
    {
        const auto& name = p.first;
        const auto& data = p.second; // std::vector<char>

        // copy into a heap MemoryBlock that we keep alive
        auto block = std::make_unique<juce::MemoryBlock>(data.data(), data.size());

        // create a MemoryInputStream that points at the block's data
        // and hand it to the builder. DO NOT delete this pointer yourself:
        // builder will delete it after writeToStream().
        auto* stream = new juce::MemoryInputStream(block->getData(), (size_t)block->getSize(), false);

        // hand stream to builder (builder will delete stream for us)
        builder.addEntry(stream,
                         0,                 // compression level
                         name,              // stored name/path
                         juce::Time::getCurrentTime());

        // keep block alive until after writeToStream()
        ownedBlocks.push_back(std::move(block));
    }

    // This is the critical call: the InputStreams we handed to builder must
    // still be valid (their backing MemoryBlocks are kept alive above).
    builder.writeToStream(memOut, nullptr);

    // now safe to destroy our blocks (ownedBlocks goes out of scope)
    return std::make_unique<juce::MemoryBlock>(memOut.getData(), memOut.getDataSize());
}

