#pragma once

#include <algorithm>
#include <cmath>

// The six waveshaping curves. Every curve has a slope of 1 at the origin,
// so with low drive the signal passes through at unity gain.
namespace vibe
{
    constexpr float kHalfPi = 1.57079632679489661923f;
    constexpr float kTwoOverPi = 0.63661977236758134308f;

    inline float analogClip (float x) noexcept   { return std::tanh (x); }

    inline float softSine (float x) noexcept
    {
        if (x >= 1.0f)  return 1.0f;
        if (x <= -1.0f) return -1.0f;
        return std::sin (kHalfPi * x);
    }

    inline float medium (float x) noexcept       { return kTwoOverPi * std::atan (kHalfPi * x); }

    inline float hard (float x) noexcept
    {
        const float x2 = x * x;
        return x / std::sqrt (std::sqrt (1.0f + x2 * x2));
    }

    // Not clamped: past full scale the waveform folds back on itself.
    inline float sinoidFold (float x) noexcept   { return std::sin (kHalfPi * x); }

    inline float digitalClip (float x) noexcept  { return std::clamp (x, -1.0f, 1.0f); }

    inline float shape (int curve, float x) noexcept
    {
        switch (curve)
        {
            case 0:  return analogClip (x);
            case 1:  return softSine (x);
            case 2:  return medium (x);
            case 3:  return hard (x);
            case 4:  return sinoidFold (x);
            default: return digitalClip (x);
        }
    }
}
