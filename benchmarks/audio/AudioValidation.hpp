#pragma once
#include <nlohmann/json.hpp>
namespace benchmark
{
    inline bool routingValid(const nlohmann::json &channels,
                             const nlohmann::json &request)
    {
        const auto routing = request.at("routing").get<std::string>();
        if (channels.size() != (routing == "stereo" ? 2u : 20u))
        {
            return false;
        }
        for (size_t c = 0; c < channels.size(); ++c)
        {
            const bool silent = request.value("idle", false) || c == 10 ||
                                c == 11 || (routing == "all" && c >= 2);
            if (silent && channels[c].at("energy").get<double>() != 0.0)
            {
                return false;
            }
        }
        if (routing == "spread" && !request.value("idle", false))
        {
            for (int c = 0; c < 8; ++c)
            {
                if (channels[2 + c].at("energy").get<double>() <= 0.0 ||
                    channels[2 + c].at("hash") != channels[12 + c].at("hash"))
                {
                    return false;
                }
            }
        }
        return true;
    }
    inline bool tailMatches(const nlohmann::json &changed,
                            const nlohmann::json &reference)
    {
        if (changed.size() != 20 || reference.size() != 20)
        {
            return false;
        }
        for (int c = 2; c < 20; ++c)
        {
            if (changed[c].at("hash") != reference[c].at("hash"))
            {
                return false;
            }
            const double energy = changed[c].at("energy").get<double>();
            if (c == 10 || c == 11)
            {
                if (energy != 0.0)
                {
                    return false;
                }
            }
            else if (energy <= 0.0)
            {
                return false;
            }
        }
        return true;
    }
} // namespace benchmark
