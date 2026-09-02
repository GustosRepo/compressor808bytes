#include "CompressorEngine.h"

#include <algorithm>
#include <cmath>

namespace compressor808bytes
{
namespace
{
constexpr float minimumLinearLevel = 1.0e-12f;
constexpr float minimumDb = -120.0f;
constexpr float pi = 3.14159265358979323846f;

float linearToDb(float level) noexcept
{
    return 20.0f * std::log10(std::max(std::abs(level), minimumLinearLevel));
}

float coefficientForMilliseconds(float milliseconds, double sampleRate, float multiplier = 1.0f) noexcept
{
    const auto seconds = std::max(0.0001f, milliseconds * multiplier * 0.001f);
    return std::exp(-1.0f / (seconds * static_cast<float>(std::max(1.0, sampleRate))));
}

float onePoleLowPassCoefficient(float cutoffHz, double sampleRate) noexcept
{
    const auto normalisedCutoff = std::clamp(cutoffHz / static_cast<float>(std::max(1.0, sampleRate)), 0.0001f, 0.45f);
    return 1.0f - std::exp(-2.0f * pi * normalisedCutoff);
}
} // namespace

void CompressorEngine::prepare(double sampleRate, int, int)
{
    sampleRateHz = std::max(1.0, sampleRate);
    for (auto& filter : sidechainFilters)
        filter.prepare(sampleRate);

    reset();
}

void CompressorEngine::reset()
{
    for (auto& filter : sidechainFilters)
        filter.reset();

    previousCharacterInput.fill(0.0f);
    characterLowPassState.fill(0.0f);
    characterStateInitialised.fill(false);
    rmsEnvelopePower = 0.0f;
    peakEnvelope = 0.0f;
    smoothedGainDb = 0.0f;
    gainReductionDb = 0.0f;
}

void CompressorEngine::setParameters(const CompressorParameters& newParameters)
{
    parameters = newParameters;
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

float CompressorEngine::processDetector(float linkedPower, float linkedPeak) noexcept
{
    const auto attackCoefficient = coefficientForMilliseconds(parameters.attackMs, sampleRateHz);
    const auto releaseCoefficient = coefficientForMilliseconds(parameters.releaseMs, sampleRateHz);
    const auto rmsCoefficient = linkedPower > rmsEnvelopePower ? attackCoefficient : releaseCoefficient;
    rmsEnvelopePower = rmsCoefficient * rmsEnvelopePower + (1.0f - rmsCoefficient) * linkedPower;

    const auto peakAttackCoefficient = coefficientForMilliseconds(std::max(0.02f, parameters.attackMs * 0.45f), sampleRateHz);
    const auto peakReleaseCoefficient = coefficientForMilliseconds(std::max(5.0f, parameters.releaseMs * 0.55f), sampleRateHz);
    const auto peakCoefficient = linkedPeak > peakEnvelope ? peakAttackCoefficient : peakReleaseCoefficient;
    peakEnvelope = peakCoefficient * peakEnvelope + (1.0f - peakCoefficient) * linkedPeak;

    const auto rmsLevel = std::sqrt(std::max(rmsEnvelopePower, 0.0f));
    const auto peakBlend = parameters.detectorMode == 1 ? 0.72f : 0.28f;
    return rmsLevel + (std::max(peakEnvelope, rmsLevel) - rmsLevel) * peakBlend;
}

float CompressorEngine::smoothGainDb(float targetGainDb) noexcept
{
    const auto attackCoefficient = coefficientForMilliseconds(std::max(0.02f, parameters.attackMs * 0.6f), sampleRateHz);

    if (targetGainDb < smoothedGainDb)
    {
        smoothedGainDb = attackCoefficient * smoothedGainDb + (1.0f - attackCoefficient) * targetGainDb;
        return smoothedGainDb;
    }

    const auto currentReductionDb = std::max(0.0f, -smoothedGainDb);
    const auto targetReductionDb = std::max(0.0f, -targetGainDb);
    const auto releaseDepth = std::clamp(currentReductionDb / 14.0f, 0.0f, 1.0f);
    const auto recoveryDistance = std::clamp((currentReductionDb - targetReductionDb) / 12.0f, 0.0f, 1.0f);
    const auto releaseMultiplier = std::clamp(0.48f + releaseDepth * 1.55f - recoveryDistance * 0.28f, 0.35f, 2.1f);
    const auto releaseCoefficient = coefficientForMilliseconds(parameters.releaseMs, sampleRateHz, releaseMultiplier);
    smoothedGainDb = releaseCoefficient * smoothedGainDb + (1.0f - releaseCoefficient) * targetGainDb;
    return smoothedGainDb;
}

float CompressorEngine::shapeCharacterSample(float sample, float drive, float asymmetry) const noexcept
{
    const auto biasedInput = sample + asymmetry;
    const auto shapedBias = std::tanh(asymmetry * drive);
    const auto normaliser = std::max(0.1f, std::tanh(drive));
    const auto shaped = (std::tanh(biasedInput * drive) - shapedBias) / normaliser;
    return std::clamp(shaped, -1.6f, 1.6f);
}

float CompressorEngine::processCharacter(float sample, int channel, float reductionDb) noexcept
{
    const auto characterMode = std::clamp(parameters.character, 0, 2);
    if (characterMode == 0)
    {
        previousCharacterInput[static_cast<size_t>(channel)] = sample;
        characterLowPassState[static_cast<size_t>(channel)] = sample;
        characterStateInitialised[static_cast<size_t>(channel)] = true;
        return sample;
    }

    const auto channelIndex = static_cast<size_t>(channel);
    if (! characterStateInitialised[channelIndex])
    {
        previousCharacterInput[channelIndex] = sample;
        characterLowPassState[channelIndex] = sample;
        characterStateInitialised[channelIndex] = true;
    }

    const auto baseDrive = characterMode == 1 ? 1.18f : 1.32f;
    const auto dynamicDrive = std::min(0.38f, reductionDb * (characterMode == 1 ? 0.012f : 0.018f));
    const auto drive = baseDrive + dynamicDrive;
    const auto asymmetry = characterMode == 1 ? 0.018f : 0.028f;
    const auto wetMix = characterMode == 1 ? 0.18f : 0.24f;
    const auto oversamplingFactor = parameters.oversampling == 2 ? 4 : (parameters.oversampling == 1 ? 2 : 1);
    const auto oversampledRate = sampleRateHz * static_cast<double>(oversamplingFactor);
    const auto lowPassCoefficient = onePoleLowPassCoefficient(std::min(18000.0f, 0.43f * static_cast<float>(sampleRateHz)), oversampledRate);

    auto accumulated = 0.0f;
    const auto previousInput = previousCharacterInput[channelIndex];
    for (int phase = 1; phase <= oversamplingFactor; ++phase)
    {
        const auto fraction = static_cast<float>(phase) / static_cast<float>(oversamplingFactor);
        const auto oversampledInput = previousInput + (sample - previousInput) * fraction;
        const auto shaped = shapeCharacterSample(oversampledInput, drive, asymmetry);
        const auto compensated = shaped * (characterMode == 1 ? 0.985f : 0.965f);
        characterLowPassState[channelIndex] += lowPassCoefficient * (compensated - characterLowPassState[channelIndex]);
        accumulated += characterLowPassState[channelIndex];
    }

    previousCharacterInput[channelIndex] = sample;
    const auto shapedSample = accumulated / static_cast<float>(oversamplingFactor);
    return sample + (shapedSample - sample) * wetMix;
}

void CompressorEngine::process(float* const* channels, int channelCount, int sampleCount) noexcept
{
    const auto activeChannels = std::clamp(channelCount, 1, 2);
    gainReductionDb = 0.0f;

    for (int sample = 0; sample < sampleCount; ++sample)
    {
        auto linkedPower = 0.0f;
        auto linkedPeak = 0.0f;
        for (int channel = 0; channel < activeChannels; ++channel)
        {
            const auto detectorSample = sidechainFilters[static_cast<size_t>(channel)].process(channels[channel][sample]);
            linkedPower += detectorSample * detectorSample;
            linkedPeak = std::max(linkedPeak, std::abs(detectorSample));
        }

        const auto detectorLevel = processDetector(linkedPower / static_cast<float>(activeChannels), linkedPeak);
        const auto targetGainDb = calculateGainDb(std::max(minimumDb, linearToDb(detectorLevel)));
        const auto gainDb = smoothGainDb(targetGainDb);
        const auto reductionDb = std::max(0.0f, -gainDb);
        const auto gain = std::pow(10.0f, (gainDb + parameters.makeupDb) / 20.0f);
        gainReductionDb = std::max(gainReductionDb, reductionDb);

        for (int channel = 0; channel < activeChannels; ++channel)
            channels[channel][sample] = processCharacter(channels[channel][sample] * gain, channel, reductionDb);
    }
}
} // namespace compressor808bytes
