#include "CompressorEngine.h"

#include <algorithm>
#include <cmath>

namespace compressor808bytes
{
namespace
{
constexpr float minimumLinearLevel = 1.0e-12f;
constexpr float minimumDb = -120.0f;

float linearToDb(float level) noexcept
{
    return 20.0f * std::log10(std::max(std::abs(level), minimumLinearLevel));
}
} // namespace

void CompressorEngine::prepare(double sampleRate, int, int)
{
    detector.prepare(sampleRate);
    for (auto& filter : sidechainFilters)
        filter.prepare(sampleRate);

    reset();
}

void CompressorEngine::reset()
{
    detector.reset();
    for (auto& filter : sidechainFilters)
        filter.reset();

    gainReductionDb = 0.0f;
}

void CompressorEngine::setParameters(const CompressorParameters& newParameters)
{
    parameters = newParameters;
    detector.setTimes(parameters.attackMs, parameters.releaseMs);
    for (auto& filter : sidechainFilters)
        filter.setCutoff(parameters.sidechainHighPassHz);
}

float CompressorEngine::calculateGainDb(float inputDb) const noexcept
{
    const auto ratio = std::max(1.0f, parameters.ratio);
    const auto slope = (1.0f / ratio) - 1.0f;
    const auto knee = std::max(0.0f, parameters.kneeDb);

    if (knee <= 0.0f)
        return std::min(0.0f, std::max(0.0f, inputDb - parameters.thresholdDb) * slope);

    const auto lowerKnee = parameters.thresholdDb - knee * 0.5f;
    const auto upperKnee = parameters.thresholdDb + knee * 0.5f;
    if (inputDb <= lowerKnee)
        return 0.0f;
    if (inputDb >= upperKnee)
        return std::min(0.0f, (inputDb - parameters.thresholdDb) * slope);

    const auto distanceIntoKnee = inputDb - lowerKnee;
    return slope * distanceIntoKnee * distanceIntoKnee / (2.0f * knee);
}

void CompressorEngine::process(float* const* channels, int channelCount, int sampleCount) noexcept
{
    const auto activeChannels = std::clamp(channelCount, 1, 2);
    gainReductionDb = 0.0f;

    for (int sample = 0; sample < sampleCount; ++sample)
    {
        auto linkedPower = 0.0f;
        for (int channel = 0; channel < activeChannels; ++channel)
        {
            const auto detectorSample = sidechainFilters[static_cast<size_t>(channel)].process(channels[channel][sample]);
            linkedPower += detectorSample * detectorSample;
        }

        const auto envelope = detector.processPower(linkedPower / static_cast<float>(activeChannels));
        const auto gainDb = calculateGainDb(std::max(minimumDb, linearToDb(envelope)));
        const auto gain = std::pow(10.0f, (gainDb + parameters.makeupDb) / 20.0f);
        gainReductionDb = std::max(gainReductionDb, -gainDb);

        for (int channel = 0; channel < activeChannels; ++channel)
            channels[channel][sample] *= gain;
    }
}
} // namespace compressor808bytes