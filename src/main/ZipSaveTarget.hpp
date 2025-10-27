#pragma once

#include "SaveTarget.hpp"

#include <mpc_fs.hpp>

#include <juce_core/juce_core.h>
#include <unordered_map>

namespace vmpc_juce {

class ZipSaveTarget : public mpc::SaveTarget
{
    std::unordered_map<std::string, std::vector<char>> files;

public:
    ZipSaveTarget() = default;

    // Construct from an existing zip blob
    explicit ZipSaveTarget(const void* data, size_t size);

    void setFileData(const fs::path& path, const std::vector<char>& data) override;

    std::vector<char> getFileData(const fs::path& path) const override;
    
    bool exists(const fs::path& path) const override;
    
    std::uintmax_t fileSize(const fs::path& path) const override;
    
    std::unique_ptr<juce::MemoryBlock> toZipMemoryBlock() const;
};

} // namespace vmpc_juce
