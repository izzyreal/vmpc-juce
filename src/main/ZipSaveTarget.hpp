#pragma once

#include "SaveTarget.hpp"

#include <mpc_fs.hpp>

#include <juce_core/juce_core.h>
#include <unordered_map>

namespace vmpc_juce
{

    class ZipSaveTarget : public mpc::SaveTarget
    {
        std::unordered_map<std::string, std::vector<char>> files;

    public:
        ZipSaveTarget() = default;

        // Construct from an existing zip blob
        explicit ZipSaveTarget(const void *data, size_t size);

        mpc_fs::result<void>
        setFileData(const mpc_fs::path &path,
                    const std::vector<char> &data) override;

        mpc_fs::result<std::vector<char>>
        getFileData(const mpc_fs::path &path) const override;

        mpc_fs::result<bool> exists(const mpc_fs::path &path) const override;

        mpc_fs::result<std::uintmax_t>
        fileSize(const mpc_fs::path &path) const override;

        std::unique_ptr<juce::MemoryBlock> toZipMemoryBlock() const;
    };

} // namespace vmpc_juce
