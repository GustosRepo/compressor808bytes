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
constexpr float allButtonsRatioThreshold = 20.5f;

float linearToDb(float level) noexcept
{
    return 20.0f * std::log10(std::max(std::abs(level), minimumLinearLevel));
}

float coefficientForMilliseconds(float milliseconds, double sampleRate, float multiplier = 1.0f) noexcept
{
    const auto seconds = std::max(0.00002f, milliseconds * multiplier * 0.001f);
    return std::exp(-1.0f / (seconds * static_cast<float>(std::max(1.0, sampleRate))));
}

float onePoleLowPassCoefficient(float cutoffHz, double sampleRate) noexcept
{
    const auto normalisedCutoff = std::clamp(cutoffHz / static_cast<float>(std::max(1.0, sampleRate)), 0.0001f, 0.45f);
    return 1.0f - std::exp(-2.0f * pi * normalisedCutoff);
}

float ratioThresholdOffsetDb(float ratio) noexcept
{
    if (ratio >= allButtonsRatioThreshold)
        return -1.8f;

    return std::max(0.0f, std::log2(std::max(1.0f, ratio / 4.0f))) * 0.55f;
}

float effectiveFeedbackRatio(float ratio, float overThresholdDb) noexcept
{
    if (ratio < allButtonsRatioThreshold)
        return std::max(1.0f, ratio);

    return 12.0f + 7.0f * (1.0f - std::exp(-std::max(0.0f, overThresholdDb) / 9.0f));
}

float feedbackSlopeScale(float ratio) noexcept
{
    return ratio >= allButtonsRatioThreshold ? 0.62f : 0.52f;
}
} // namespace

void CompressorEngine::prepare(double sampleRate, int, int)
{
    sampleRateHz = std::max(1.0, sampleRate);
    for (auto& filter : sidechainFilters)
        filter.prepare(sampleRate);

    for (size_t channel = 0; channel < sidechainFilters.size(); ++channel)
    {
        characterOversamplers2x[channel] = std::make_unique<juce::dsp::Oversampling<float>>(
            1, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
        characterOversamplers4x[channel] = std::make_unique<juce::dsp::Oversampling<float>>(
            1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
        characterOversamplers2x[channel]->initProcessing(1);
        characterOversamplers4x[channel]->initProcessing(1);
    }

    reset();
}

void CompressorEngine::reset()
{
    for (auto& filter : sidechainFilters)
        filter.reset();

    feedbackSamples.fill(0.0f);
    characterLowBandState.fill(0.0f);
    characterStateInitialised.fill(false);
    for (auto& oversampler : characterOversamplers2x)
        if (oversampler != nullptr)
            oversampler->reset();
    for (auto& oversampler : characterOversamplers4x)
        if (oversampler != nullptr)
            oversampler->reset();
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
    const auto thresholdDb = parameters.thresholdDb + ratioThresholdOffsetDb(ratio);
    const auto knee = std::clamp(parameters.kneeDb, 0.0f, 8.0f);

    const auto overThresholdDb = inputDb - thresholdDb;
    auto shapedOverDb = 0.0f;

    if (knee <= 0.0f)
    {
        shapedOverDb = std::max(0.0f, overThresholdDb);
    }
    else
    {
        const auto lowerKnee = -knee * 0.5f;
        const auto upperKnee = knee * 0.5f;
        if (overThresholdDb <= lowerKnee)
            shapedOverDb = 0.0f;
        else if (overThresholdDb >= upperKnee)
            shapedOverDb = overThresholdDb;
        else
        {
            const auto distanceIntoKnee = overThresholdDb - lowerKnee;
            shapedOverDb = distanceIntoKnee * distanceIntoKnee / (2.0f * knee);
        }
    }

    const auto feedbackRatio = effectiveFeedbackRatio(ratio, shapedOverDb);
    const auto slope = (1.0f - feedbackRatio) * feedbackSlopeScale(ratio);
    return std::min(0.0f, shapedOverDb * slope);
}

float CompressorEngine::processDetector(float linkedPower, float linkedPeak) noexcept
{
    const auto allButtons = parameters.ratio >= allButtonsRatioThreshold;
    const auto attackCoefficient = coefficientForMilliseconds(parameters.attackMs * (allButtons ? 1.90f : 1.35f), sampleRateHz);
    const auto releaseCoefficient = coefficientForMilliseconds(parameters.releaseMs * (allButtons ? 0.58f : 0.72f), sampleRateHz);
    const auto rmsCoefficient = linkedPower > rmsEnvelopePower ? attackCoefficient : releaseCoefficient;
    rmsEnvelopePower = rmsCoefficient * rmsEnvelopePower + (1.0f - rmsCoefficient) * linkedPower;

    const auto fastDetector = parameters.detectorMode == 1;
    const auto peakAttackMultiplier = (fastDetector ? 0.16f : 0.32f) * (allButtons ? 2.10f : 1.0f);
    const auto peakReleaseMultiplier = (fastDetector ? 0.50f : 0.36f) * (allButtons ? 0.72f : 1.0f);
    const auto peakAttackCoefficient = coefficientForMilliseconds(std::max(0.02f, parameters.attackMs * peakAttackMultiplier), sampleRateHz);
    const auto peakReleaseCoefficient = coefficientForMilliseconds(std::max(18.0f, parameters.releaseMs * peakReleaseMultiplier), sampleRateHz);
    const auto peakCoefficient = linkedPeak > peakEnvelope ? peakAttackCoefficient : peakReleaseCoefficient;
    peakEnvelope = peakCoefficient * peakEnvelope + (1.0f - peakCoefficient) * linkedPeak;

    const auto rmsLevel = std::sqrt(std::max(rmsEnvelopePower, 0.0f));
    const auto peakBlend = allButtons ? (fastDetector ? 0.93f : 0.70f) : (fastDetector ? 0.98f : 0.58f);
    return rmsLevel + (std::max(peakEnvelope, rmsLevel) - rmsLevel) * peakBlend;
}

float CompressorEngine::smoothGainDb(float targetGainDb) noexcept
{
    const auto allButtons = parameters.ratio >= allButtonsRatioThreshold;
    const auto attackMultiplier = allButtons ? 0.72f : 0.28f;
    const auto attackCoefficient = coefficientForMilliseconds(std::max(0.02f, parameters.attackMs * attackMultiplier), sampleRateHz);

    if (targetGainDb < smoothedGainDb)
    {
        smoothedGainDb = attackCoefficient * smoothedGainDb + (1.0f - attackCoefficient) * targetGainDb;
        return smoothedGainDb;
    }

    const auto currentReductionDb = std::max(0.0f, -smoothedGainDb);
    const auto targetReductionDb = std::max(0.0f, -targetGainDb);
    const auto releaseDepth = std::clamp(currentReductionDb / 18.0f, 0.0f, 1.0f);
    const auto recoveryDistance = std::clamp((currentReductionDb - targetReductionDb) / 10.0f, 0.0f, 1.0f);
    const auto allButtonsBias = allButtons ? 0.18f : 0.0f;
    const auto recoveryPull = allButtons ? 0.48f : 0.16f;
    const auto releaseMultiplier = std::clamp(0.22f + allButtonsBias + releaseDepth * 1.18f - recoveryDistance * recoveryPull, 0.14f, 1.55f);
    const auto releaseCoefficient = coefficientForMilliseconds(parameters.releaseMs, sampleRateHz, releaseMultiplier);
    smoothedGainDb = releaseCoefficient * smoothedGainDb + (1.0f - releaseCoefficient) * targetGainDb;
    return smoothedGainDb;
}

float CompressorEngine::processFetAttenuator(float sample, float gain, float reductionDb) const noexcept
{
    const auto attenuated = sample * gain;
    const auto control = std::clamp(reductionDb / 24.0f, 0.0f, 1.0f);
    if (control <= 0.0f)
        return attenuated;

    const auto allButtons = parameters.ratio >= allButtonsRatioThreshold;
    const auto drive = (allButtons ? 1.18f : 0.72f) + control * (allButtons ? 1.45f : 0.62f);
    const auto asymmetry = allButtons ? 0.034f : 0.014f;
    const auto shaped = shapeCharacterSample(attenuated, drive, asymmetry);
    const auto mix = std::clamp((allButtons ? 0.24f : 0.08f) + control * (allButtons ? 0.30f : 0.14f), 0.0f, 0.58f);
    return attenuated + (shaped - attenuated) * mix;
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
        characterStateInitialised[static_cast<size_t>(channel)] = true;
        characterLowBandState[static_cast<size_t>(channel)] = sample;
        return sample;
    }

    const auto channelIndex = static_cast<size_t>(channel);
    if (! characterStateInitialised[channelIndex])
    {
        characterLowBandState[channelIndex] = sample;
        characterStateInitialised[channelIndex] = true;
    }

    const auto allButtons = parameters.ratio >= allButtonsRatioThreshold;
    const auto baseDrive = (characterMode == 1 ? 1.28f : 1.56f) + (allButtons ? 0.18f : 0.0f);
    const auto dynamicDrive = std::min(allButtons ? 0.76f : 0.52f, reductionDb * (characterMode == 1 ? 0.016f : 0.026f));
    const auto drive = baseDrive + dynamicDrive;
    const auto asymmetry = (characterMode == 1 ? 0.021f : 0.038f) + (allButtons ? 0.009f : 0.0f);
    const auto wetMix = (characterMode == 1 ? 0.20f : 0.34f) + (allButtons ? 0.08f : 0.0f);
    const auto oversamplingStages = std::clamp(parameters.oversampling, 0, 2);
    const auto oversampledRate = sampleRateHz * static_cast<double>(1 << oversamplingStages);
    const auto toneCoefficient = onePoleLowPassCoefficient(characterMode == 1 ? 150.0f : 210.0f, oversampledRate);
    const auto shapedSample = oversamplingStages == 0
        ? processCharacterSample(sample, channel, characterMode, drive, asymmetry, toneCoefficient)
        : processOversampledCharacter(sample, channel, characterMode, drive, asymmetry, toneCoefficient, oversamplingStages);
    return sample + (shapedSample - sample) * wetMix;
}

float CompressorEngine::processCharacterSample(float sample, int channel, int characterMode, float drive, float asymmetry,
                                               float toneCoefficient) noexcept
{
    const auto channelIndex = static_cast<size_t>(channel);
    characterLowBandState[channelIndex] += toneCoefficient * (sample - characterLowBandState[channelIndex]);
    const auto lowBand = characterLowBandState[channelIndex];
    const auto highBand = sample - lowBand;
    const auto toneDrivenInput = sample + lowBand * (characterMode == 1 ? 0.035f : 0.060f) - highBand * (characterMode == 1 ? 0.010f : 0.018f);
    const auto toneDrive = drive * (1.0f + std::min(0.18f, std::abs(lowBand) * (characterMode == 1 ? 0.16f : 0.24f)));
    const auto shaped = shapeCharacterSample(toneDrivenInput, toneDrive, asymmetry);
    return shaped * (characterMode == 1 ? 0.985f : 0.965f);
}

float CompressorEngine::processOversampledCharacter(float sample, int channel, int characterMode, float drive, float asymmetry,
                                                    float toneCoefficient, int oversamplingStages) noexcept
{
    auto* oversampler = oversamplerFor(channel, oversamplingStages);
    if (oversampler == nullptr)
        return processCharacterSample(sample, channel, characterMode, drive, asymmetry, toneCoefficient);

    const float* inputPointers[] { &sample };
    juce::dsp::AudioBlock<const float> inputBlock(inputPointers, 1, 1);
    auto oversampledBlock = oversampler->processSamplesUp(inputBlock);
    auto* oversampledSamples = oversampledBlock.getChannelPointer(0);

    for (size_t index = 0; index < oversampledBlock.getNumSamples(); ++index)
        oversampledSamples[index] = processCharacterSample(oversampledSamples[index], channel, characterMode, drive, asymmetry, toneCoefficient);

    auto outputSample = 0.0f;
    float* outputPointers[] { &outputSample };
    juce::dsp::AudioBlock<float> outputBlock(outputPointers, 1, 1);
    oversampler->processSamplesDown(outputBlock);
    return outputSample;
}

juce::dsp::Oversampling<float>* CompressorEngine::oversamplerFor(int channel, int oversamplingStages) noexcept
{
    const auto channelIndex = static_cast<size_t>(std::clamp(channel, 0, 1));
    if (oversamplingStages == 1)
        return characterOversamplers2x[channelIndex].get();
    if (oversamplingStages == 2)
        return characterOversamplers4x[channelIndex].get();
    return nullptr;
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
            const auto channelIndex = static_cast<size_t>(channel);
            const auto detectorSample = sidechainFilters[channelIndex].process(feedbackSamples[channelIndex]);
            linkedPower += detectorSample * detectorSample;
            linkedPeak = std::max(linkedPeak, std::abs(detectorSample));
        }

        const auto detectorLevel = processDetector(linkedPower / static_cast<float>(activeChannels), linkedPeak);
        const auto targetGainDb = calculateGainDb(std::max(minimumDb, linearToDb(detectorLevel)));
        const auto gainDb = smoothGainDb(targetGainDb);
        const auto reductionDb = std::max(0.0f, -gainDb);
        const auto gain = std::pow(10.0f, gainDb / 20.0f);
        const auto makeupGain = std::pow(10.0f, parameters.makeupDb / 20.0f);
        gainReductionDb = std::max(gainReductionDb, reductionDb);

        for (int channel = 0; channel < activeChannels; ++channel)
        {
            const auto channelIndex = static_cast<size_t>(channel);
            const auto fetOutput = processFetAttenuator(channels[channel][sample], gain, reductionDb);
            feedbackSamples[channelIndex] = fetOutput;
            channels[channel][sample] = processCharacter(fetOutput * makeupGain, channel, reductionDb);
        }
    }
}
} // namespace compressor808bytes
