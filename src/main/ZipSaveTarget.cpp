#include "ZipSaveTarget.hpp"

using namespace vmpc_juce;

ZipSaveTarget::ZipSaveTarget(const void *data, size_t size)
{
    juce::MemoryInputStream input(data, size, false);
    juce::ZipFile zip(input);

    for (int i = 0; i < zip.getNumEntries(); ++i)
    {
        auto *entry = zip.getEntry(i);
        if (!entry)
        {
            continue;
        }

        std::unique_ptr<juce::InputStream> stream(zip.createStreamForEntry(i));
        if (!stream)
        {
            continue;
        }

        juce::MemoryOutputStream memOut;
        memOut.writeFromInputStream(*stream, -1);
        const auto bytes = memOut.getMemoryBlock();
        const auto filename = entry->filename.toStdString();

        files[filename] =
            std::vector<char>((const char *)bytes.getData(),
                              (const char *)bytes.getData() + bytes.getSize());
    }
}

mpc_fs::result<void>
ZipSaveTarget::setFileData(const mpc_fs::path &path,
                           const std::vector<char> &data)
{
    const auto key = path.string();
    if (data.empty())
    {
        files.erase(key);
    }
    else
    {
        files[key] = std::vector<char>(data.begin(), data.end());
    }
    return {};
}

mpc_fs::result<std::vector<char>>
ZipSaveTarget::getFileData(const mpc_fs::path &path) const
{
    const auto key = path.string();
    auto it = files.find(key);
    if (it == files.end())
    {
        return tl::unexpected(mpc_fs::make_error(
            "zip_get_file_data", path,
            std::make_error_code(std::errc::no_such_file_or_directory)));
    }
    return it->second;
}

mpc_fs::result<bool> ZipSaveTarget::exists(const mpc_fs::path &path) const
{
    return files.find(path.string()) != files.end();
}

mpc_fs::result<std::uintmax_t>
ZipSaveTarget::fileSize(const mpc_fs::path &path) const
{
    auto it = files.find(path.string());
    if (it == files.end())
    {
        return tl::unexpected(mpc_fs::make_error(
            "zip_file_size", path,
            std::make_error_code(std::errc::no_such_file_or_directory)));
    }
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

    for (const auto &p : files)
    {
        const auto &name = p.first;
        const auto &data = p.second; // std::vector<char>

        // copy into a heap MemoryBlock that we keep alive
        auto block =
            std::make_unique<juce::MemoryBlock>(data.data(), data.size());

        // create a MemoryInputStream that points at the block's data
        // and hand it to the builder. DO NOT delete this pointer yourself:
        // builder will delete it after writeToStream().
        auto *stream = new juce::MemoryInputStream(
            block->getData(), (size_t)block->getSize(), false);

        // hand stream to builder (builder will delete stream for us)
        builder.addEntry(stream,
                         0,    // compression level
                         name, // stored name/path
                         juce::Time::getCurrentTime());

        // keep block alive until after writeToStream()
        ownedBlocks.push_back(std::move(block));
    }

    // This is the critical call: the InputStreams we handed to builder must
    // still be valid (their backing MemoryBlocks are kept alive above).
    builder.writeToStream(memOut, nullptr);

    // now safe to destroy our blocks (ownedBlocks goes out of scope)
    return std::make_unique<juce::MemoryBlock>(memOut.getData(),
                                               memOut.getDataSize());
}
