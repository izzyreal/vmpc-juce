#pragma once

#include <limits>
#include <algorithm>

inline bool nearlyEqual(float a, float b, float relTol = std::numeric_limits<float>::epsilon())
{
    return std::fabs(a - b) <= relTol * std::max({1.0f, std::fabs(a), std::fabs(b)});
}

inline bool nearlyEqual(double a, double b, double relTol = std::numeric_limits<double>::epsilon())
{
    return std::abs(a - b) <= relTol * std::max({1.0, std::abs(a), std::abs(b)});
}

