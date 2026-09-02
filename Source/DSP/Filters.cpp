#include "Filters.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace compressor808bytes
{
void SidechainHighPassFilter::prepare(double sampleRate)
{
    sampleRateHz = std::max(1.0, sampleRate);
    reset();
    setCutoff(20.0f);
}

void SidechainHighPassFilter::reset()
{
    previousInput = 0.0f;
    previousOutput = 0.0f;
}

void SidechainHighPassFilter::setCutoff(float frequencyHz)
{
    const auto cutoff = std::clamp(frequencyHz, 20.0f, 500.0f);
    const auto rc = 1.0f / (2.0f * std::numbers::pi_v<float> * cutoff);
    const auto dt = 1.0f / static_cast<float>(sampleRateHz);
    coefficient = rc / (rc + dt);
}

float SidechainHighPassFilter::process(float sample) noexcept
{
    const auto output = coefficient * (previousOutput + sample - previousInput);
    previousInput = sample;
    previousOutput = output;
    return output;
}
} // namespace compressor808bytes