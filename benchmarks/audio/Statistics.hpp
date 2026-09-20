#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>
namespace benchmark
{
    struct Comparison
    {
        double before, after, deltaPercent, low, high;
        std::string verdict;
    };
    inline Comparison compare(const std::vector<double> &before,
                              const std::vector<double> &after,
                              double relativeLimit, double absoluteLimit)
    {
        if (before.size() != after.size() || before.size() < 2)
        {
            throw std::runtime_error("Need at least two paired repetitions");
        }
        std::vector<double> margins;
        for (size_t i = 0; i < before.size(); ++i)
        {
            if (!std::isfinite(before[i]) || !std::isfinite(after[i]) ||
                before[i] < 0 || after[i] < 0)
            {
                throw std::runtime_error("Invalid measurement");
            }
            margins.push_back(
                after[i] - before[i] -
                std::max(relativeLimit * before[i], absoluteLimit));
        }
        auto mean = [](const auto &values)
        {
            return std::accumulate(values.begin(), values.end(), 0.0) /
                   values.size();
        };
        // Deterministic paired bootstrap of the mean threshold margin; 95%
        // interval.
        uint64_t random = 0x9e3779b97f4a7c15ULL;
        std::vector<double> bootstrap(10000);
        for (auto &value : bootstrap)
        {
            double sum = 0;
            for (size_t i = 0; i < margins.size(); ++i)
            {
                random ^= random << 13;
                random ^= random >> 7;
                random ^= random << 17;
                sum += margins[random % margins.size()];
            }
            value = sum / margins.size();
        }
        std::sort(bootstrap.begin(), bootstrap.end());
        const double low = bootstrap[249], high = bootstrap[9749];
        const double b = mean(before), a = mean(after);
        return {
            b,
            a,
            b > 0 ? (a / b - 1) * 100 : 0,
            low,
            high,
            low > 0 ? "REGRESSION" : high <= 0 ? "ACCEPTABLE" : "INCONCLUSIVE"};
    }
    inline std::string headroom(double p99, double deadline, uint64_t overruns,
                                double limit)
    {
        if (p99 >= deadline)
        {
            return "INSUFFICIENT";
        }
        if (p99 >= deadline * limit)
        {
            return "LIMITED";
        }
        if (overruns != 0)
        {
            return "RECHECK OVERRUNS";
        }
        return "COMFORTABLE";
    }
} // namespace benchmark
