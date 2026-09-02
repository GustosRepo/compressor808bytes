#include "EnvelopeDetector.h"

#include <algorithm>
#include <cmath>

namespace compressor808bytes
{
void EnvelopeDetector::prepare(double sampleRate)
{
    sampleRateHz = std::max(1.0, sampleRate);
    reset();
    setTimes(10.0f, 100.0f);
}

void EnvelopeDetector::reset()
{
    envelopePower = 0.0f;
}

void EnvelopeDetector::setTimes(float attackMs, float releaseMs)
{
    const auto coefficientFor = [this] (float milliseconds)
    {
        const auto seconds = std::max(0.0001f, milliseconds * 0.001f);
        return std::exp(-1.0f / (seconds * static_cast<float>(sampleRateHz)));
    };

    attackCoefficient = coefficientFor(attackMs);
    releaseCoefficient = coefficientFor(releaseMs);
}

float EnvelopeDetector::processPower(float power) noexcept
{
    const auto coefficient = power > envelopePower ? attackCoefficient : releaseCoefficient;
    envelopePower = coefficient * envelopePower + (1.0f - coefficient) * power;
    return std::sqrt(std::max(envelopePower, 0.0f));
}
} // namespace compressor808bytes