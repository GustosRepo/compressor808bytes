#include "DSP/CompressorEngine.h"
#include "DSP/MeterBallistics.h"
#include "PluginProcessor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <string_view>
#include <vector>

using namespace compressor808bytes;

namespace
{
constexpr float decibelTolerance = 0.15f;
constexpr float pi = 3.14159265358979323846f;

bool approximatelyEqual(float first, float second, float tolerance = decibelTolerance)
{
    return std::abs(first - second) <= tolerance;
}

float gainToDecibels(float gain)
{
    return 20.0f * std::log10(std::max(std::abs(gain), 1.0e-12f));
}

bool check(bool condition, std::string_view description)
{
    if (!condition)
        std::cerr << "Failed: " << description << '\n';

    return condition;
}

bool finiteBuffer(const std::vector<float>& buffer)
{
    for (const auto sample : buffer)
        if (!std::isfinite(sample))
            return false;

    return true;
}

float peakMagnitude(const std::vector<float>& buffer)
{
    auto peak = 0.0f;
    for (const auto sample : buffer)
        peak = std::max(peak, std::abs(sample));

    return peak;
}

float residualRms(const std::vector<float>& first, const std::vector<float>& second)
{
    auto sum = 0.0;
    for (size_t sample = 0; sample < first.size(); ++sample)
    {
        const auto difference = static_cast<double>(first[sample] - second[sample]);
        sum += difference * difference;
    }

    return static_cast<float>(std::sqrt(sum / static_cast<double>(first.size())));
}

void setProcessorParameter(CompressorAudioProcessor& processor, const char* id, float value)
{
    if (auto* parameter = processor.parameters.getParameter(id))
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}

void setFetProcessorDefaults(CompressorAudioProcessor& processor)
{
    setProcessorParameter(processor, "input", 0.0f);
    setProcessorParameter(processor, "threshold", -18.0f);
    setProcessorParameter(processor, "ratio", 8.0f);
    setProcessorParameter(processor, "attack", 0.05f);
    setProcessorParameter(processor, "release", 220.0f);
    setProcessorParameter(processor, "makeup", 0.0f);
    setProcessorParameter(processor, "mix", 100.0f);
    setProcessorParameter(processor, "output", 0.0f);
    setProcessorParameter(processor, "knee", 1.5f);
    setProcessorParameter(processor, "sidechainHPF", 30.0f);
    setProcessorParameter(processor, "detectorMode", 1.0f);
    setProcessorParameter(processor, "character", 0.0f);
    setProcessorParameter(processor, "oversampling", 0.0f);
    setProcessorParameter(processor, "bypass", 0.0f);
}

void fillProcessorTestSignal(juce::AudioBuffer<float>& buffer, float amplitude)
{
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto value = sample % 2 == 0 ? amplitude : -amplitude;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.setSample(channel, sample, value);
    }
}

float peakMagnitude(const juce::AudioBuffer<float>& buffer)
{
    auto peak = 0.0f;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        peak = std::max(peak, buffer.getMagnitude(channel, 0, buffer.getNumSamples()));

    return peak;
}

void processRepeatedProcessorBlocks(CompressorAudioProcessor& processor, juce::AudioBuffer<float>& buffer,
                                    float amplitude, int blocks)
{
    juce::MidiBuffer midi;
    for (int block = 0; block < blocks; ++block)
    {
        fillProcessorTestSignal(buffer, amplitude);
        processor.processBlock(buffer, midi);
    }
}

void processSignal(CompressorEngine& engine, std::vector<float>& left, std::vector<float>& right, float amplitude, int passes)
{
    float* channels[] { left.data(), right.data() };
    for (int pass = 0; pass < passes; ++pass)
    {
        for (size_t sample = 0; sample < left.size(); ++sample)
        {
            const auto input = sample % 2 == 0 ? amplitude : -amplitude;
            left[sample] = input;
            right[sample] = input;
        }
        engine.process(channels, 2, static_cast<int>(left.size()));
    }
}

bool testProcessingMatrix(double sampleRate, int blockSize)
{
    CompressorEngine engine;
    engine.prepare(sampleRate, blockSize, 2);
    CompressorParameters parameters;
    parameters.thresholdDb = -20.0f;
    parameters.ratio = 4.0f;
    parameters.attackMs = 0.1f;
    parameters.releaseMs = 2000.0f;
    parameters.kneeDb = 6.0f;
    engine.setParameters(parameters);

    std::vector<float> left(static_cast<size_t>(blockSize), 1.0f);
    std::vector<float> right(static_cast<size_t>(blockSize), 1.0f);
    processSignal(engine, left, right, 1.0f, 20);

    return check(finiteBuffer(left) && finiteBuffer(right), "finite output at extreme timing settings")
        && check(engine.getGainReductionDb() > 10.0f, "high-level signal receives gain reduction")
        && check(approximatelyEqual(left.back(), right.back(), 0.0001f), "linked stereo applies equal gain");
}

bool testCompressionBehavior()
{
    constexpr int blockSize = 256;
    CompressorEngine engine;
    engine.prepare(48000.0, blockSize, 2);
    CompressorParameters parameters;
    parameters.thresholdDb = -20.0f;
    parameters.ratio = 4.0f;
    parameters.attackMs = 0.1f;
    parameters.releaseMs = 100.0f;
    engine.setParameters(parameters);

    std::vector<float> left(blockSize, 1.0f);
    std::vector<float> right(blockSize, 1.0f);
    processSignal(engine, left, right, 1.0f, 20);
    const auto compressedGainDb = gainToDecibels(left.back());

    return check(compressedGainDb < -14.0f && compressedGainDb > -18.0f, "feedback 4:1 ratio settles near the expected reduction curve")
        && check(engine.getGainReductionDb() > 14.0f && engine.getGainReductionDb() < 19.0f, "gain-reduction meter reports settled feedback reduction");
}

bool testNoCompressionBelowThresholdAndUnityRatio()
{
    constexpr int blockSize = 512;
    CompressorEngine engine;
    engine.prepare(48000.0, blockSize, 2);
    CompressorParameters parameters;
    parameters.thresholdDb = -6.0f;
    parameters.ratio = 8.0f;
    parameters.attackMs = 0.1f;
    engine.setParameters(parameters);
    std::vector<float> left(blockSize, 0.1f);
    std::vector<float> right(blockSize, 0.1f);
    processSignal(engine, left, right, 0.1f, 8);
    const auto belowThresholdIsDry = approximatelyEqual(std::abs(left.back()), 0.1f, 0.001f);

    engine.reset();
    parameters.thresholdDb = -60.0f;
    parameters.ratio = 1.0f;
    engine.setParameters(parameters);
    processSignal(engine, left, right, 0.5f, 8);

    return check(belowThresholdIsDry, "signal below threshold is unaffected")
        && check(approximatelyEqual(std::abs(left.back()), 0.5f, 0.001f), "1:1 ratio produces no compression");
}

bool testSoftKneeAndSidechainHighPass()
{
    constexpr int blockSize = 512;
    CompressorEngine engine;
    engine.prepare(48000.0, blockSize, 2);
    CompressorParameters parameters;
    parameters.thresholdDb = -20.0f;
    parameters.ratio = 4.0f;
    parameters.attackMs = 0.1f;
    parameters.kneeDb = 12.0f;
    engine.setParameters(parameters);

    const auto nearThreshold = std::pow(10.0f, -20.0f / 20.0f);
    std::vector<float> left(blockSize, nearThreshold);
    std::vector<float> right(blockSize, nearThreshold);
    processSignal(engine, left, right, nearThreshold, 12);
    const auto kneeGainDb = gainToDecibels(left.back() / nearThreshold);

    engine.reset();
    parameters.sidechainHighPassHz = 500.0f;
    engine.setParameters(parameters);
    processSignal(engine, left, right, 0.5f, 1);

    return check(kneeGainDb < 0.0f && kneeGainDb > -3.0f, "soft knee transitions smoothly at threshold")
        && check(approximatelyEqual(left.back(), right.back(), 0.0001f) && finiteBuffer(left), "sidechain HPF keeps audible stereo path stable");
}

bool testGainReductionMeterBallistics()
{
    constexpr auto sampleRate = 48000.0;
    constexpr auto targetDb = 12.0f;
    constexpr auto samplesForVuRise = static_cast<int>(sampleRate * GainReductionMeterBallistics::vuRiseTimeTo99PercentSeconds);

    GainReductionMeterBallistics meter;
    meter.prepare(sampleRate);
    meter.reset();
    for (int sample = 0; sample < samplesForVuRise; ++sample)
        meter.processSample(targetDb);

    const auto expectedRiseValue = targetDb * 0.99f;
    const auto riseIsCalibrated = approximatelyEqual(meter.getValueDb(), expectedRiseValue, 0.02f);

    for (int sample = 0; sample < samplesForVuRise; ++sample)
        meter.processSample(0.0f);

    const auto expectedReleaseValue = targetDb * 0.0099f;
    const auto releaseIsCalibrated = approximatelyEqual(meter.getValueDb(), expectedReleaseValue, 0.02f);
    meter.reset();
    for (int sample = 0; sample < samplesForVuRise; ++sample)
        meter.processSample(-targetDb);

    const auto signedRiseIsCalibrated = approximatelyEqual(meter.getValueDb(), -expectedRiseValue, 0.02f);
    return check(riseIsCalibrated, "gain-reduction meter reaches 99 percent after 300 ms")
        && check(releaseIsCalibrated, "gain-reduction meter release uses the same calibrated VU timing")
        && check(signedRiseIsCalibrated, "gain-change meter supports negative values with calibrated timing");
}

float gainReductionForSparseTransientSignal(int detectorMode)
{
    constexpr int blockSize = 512;
    CompressorEngine engine;
    engine.prepare(48000.0, blockSize, 2);
    CompressorParameters parameters;
    parameters.thresholdDb = -35.0f;
    parameters.ratio = 8.0f;
    parameters.attackMs = 0.05f;
    parameters.releaseMs = 250.0f;
    parameters.detectorMode = detectorMode;
    engine.setParameters(parameters);

    std::vector<float> left(blockSize, 0.0f);
    std::vector<float> right(blockSize, 0.0f);
    float* channels[] { left.data(), right.data() };
    for (int pass = 0; pass < 20; ++pass)
    {
        for (size_t sample = 0; sample < left.size(); ++sample)
        {
            left[sample] = sample % 16 == 0 ? 1.0f : 0.0f;
            right[sample] = 0.0f;
        }
        engine.process(channels, 2, static_cast<int>(left.size()));
    }

    return engine.getGainReductionDb();
}

bool testHybridDetectorModes()
{
    const auto vintageReductionDb = gainReductionForSparseTransientSignal(0);
    const auto fastReductionDb = gainReductionForSparseTransientSignal(1);

    return check(fastReductionDb > vintageReductionDb + 0.6f, "fast detector clamps sparse transients harder than vintage mode");
}

float releaseAfterSettledCompression(float amplitude)
{
    constexpr int blockSize = 512;
    CompressorEngine engine;
    engine.prepare(48000.0, blockSize, 2);
    CompressorParameters parameters;
    parameters.thresholdDb = -35.0f;
    parameters.ratio = 8.0f;
    parameters.attackMs = 0.1f;
    parameters.releaseMs = 350.0f;
    engine.setParameters(parameters);

    std::vector<float> left(blockSize, 0.0f);
    std::vector<float> right(blockSize, 0.0f);
    processSignal(engine, left, right, amplitude, 24);
    processSignal(engine, left, right, 0.001f, 1);
    return engine.getGainReductionDb();
}

bool testProgramDependentRelease()
{
    const auto lightReleaseDb = releaseAfterSettledCompression(0.04f);
    const auto deepReleaseDb = releaseAfterSettledCompression(1.0f);

    return check(deepReleaseDb > lightReleaseDb + 8.0f, "program-dependent release holds deeper compression longer than light compression")
        && check(lightReleaseDb > 0.0f && deepReleaseDb > 0.0f, "release remains smooth instead of snapping to zero");
}

float settledReductionForRatio(float ratio)
{
    constexpr int blockSize = 512;
    CompressorEngine engine;
    engine.prepare(48000.0, blockSize, 2);
    CompressorParameters parameters;
    parameters.thresholdDb = -28.0f;
    parameters.ratio = ratio;
    parameters.attackMs = 0.05f;
    parameters.releaseMs = 180.0f;
    parameters.detectorMode = 1;
    engine.setParameters(parameters);

    std::vector<float> left(blockSize, 0.0f);
    std::vector<float> right(blockSize, 0.0f);
    processSignal(engine, left, right, 0.75f, 16);
    return engine.getGainReductionDb();
}

bool testFetRatioExtremes()
{
    const auto fourToOneReductionDb = settledReductionForRatio(4.0f);
    const auto twentyToOneReductionDb = settledReductionForRatio(20.0f);
    const auto allButtonsReductionDb = settledReductionForRatio(21.0f);

    return check(twentyToOneReductionDb > fourToOneReductionDb + 4.0f, "20:1 ratio produces substantially more FET limiting than 4:1")
        && check(allButtonsReductionDb > twentyToOneReductionDb + 0.5f, "all-buttons-style ratio is more aggressive than 20:1");
}

float settledReductionWithMakeup(float makeupDb)
{
    constexpr int blockSize = 512;
    CompressorEngine engine;
    engine.prepare(48000.0, blockSize, 2);
    CompressorParameters parameters;
    parameters.thresholdDb = -28.0f;
    parameters.ratio = 8.0f;
    parameters.attackMs = 0.05f;
    parameters.releaseMs = 250.0f;
    parameters.makeupDb = makeupDb;
    parameters.detectorMode = 1;
    engine.setParameters(parameters);

    std::vector<float> left(blockSize, 0.0f);
    std::vector<float> right(blockSize, 0.0f);
    processSignal(engine, left, right, 0.65f, 18);
    return engine.getGainReductionDb();
}

bool testOutputGainDoesNotDriveFeedbackSidechain()
{
    const auto unityMakeupReductionDb = settledReductionWithMakeup(0.0f);
    const auto hotMakeupReductionDb = settledReductionWithMakeup(12.0f);

    return check(std::abs(unityMakeupReductionDb - hotMakeupReductionDb) < 0.25f,
                 "makeup/output gain does not drive the feedback sidechain");
}

bool testCharacterStageAndOversampling()
{
    constexpr int blockSize = 2048;
    constexpr auto sampleRate = 48000.0;
    CompressorEngine cleanEngine;
    CompressorEngine characterEngine;
    cleanEngine.prepare(sampleRate, blockSize, 2);
    characterEngine.prepare(sampleRate, blockSize, 2);

    CompressorParameters parameters;
    parameters.thresholdDb = 0.0f;
    parameters.ratio = 1.0f;
    parameters.character = 0;
    cleanEngine.setParameters(parameters);
    parameters.character = 1;
    parameters.oversampling = 2;
    characterEngine.setParameters(parameters);

    std::vector<float> cleanLeft(blockSize, 0.0f);
    std::vector<float> cleanRight(blockSize, 0.0f);
    std::vector<float> characterLeft(blockSize, 0.0f);
    std::vector<float> characterRight(blockSize, 0.0f);
    for (int pass = 0; pass < 4; ++pass)
    {
        for (int sample = 0; sample < blockSize; ++sample)
        {
            const auto input = 0.45f * std::sin(2.0f * pi * 1000.0f * static_cast<float>(sample + pass * blockSize) / static_cast<float>(sampleRate));
            cleanLeft[static_cast<size_t>(sample)] = input;
            cleanRight[static_cast<size_t>(sample)] = input;
            characterLeft[static_cast<size_t>(sample)] = input;
            characterRight[static_cast<size_t>(sample)] = input;
        }

        float* cleanChannels[] { cleanLeft.data(), cleanRight.data() };
        float* characterChannels[] { characterLeft.data(), characterRight.data() };
        cleanEngine.process(cleanChannels, 2, blockSize);
        characterEngine.process(characterChannels, 2, blockSize);
    }

    const auto tonalChange = residualRms(cleanLeft, characterLeft);
    return check(finiteBuffer(characterLeft) && finiteBuffer(characterRight), "character stage stays finite")
        && check(tonalChange > 0.001f, "character stage changes the waveform")
        && check(tonalChange < 0.05f, "character stage remains subtle")
        && check(peakMagnitude(characterLeft) < 0.58f, "character stage remains bounded without obvious clipping");
}

bool testOversamplingModesAreFunctional()
{
    constexpr int blockSize = 2048;
    constexpr auto sampleRate = 48000.0;
    std::array<std::vector<float>, 3> outputs {
        std::vector<float>(blockSize, 0.0f),
        std::vector<float>(blockSize, 0.0f),
        std::vector<float>(blockSize, 0.0f)
    };

    for (int mode = 0; mode < 3; ++mode)
    {
        CompressorEngine engine;
        engine.prepare(sampleRate, blockSize, 2);
        CompressorParameters parameters;
        parameters.thresholdDb = 0.0f;
        parameters.ratio = 1.0f;
        parameters.character = 2;
        parameters.oversampling = mode;
        engine.setParameters(parameters);

        std::vector<float> right(blockSize, 0.0f);
        for (int pass = 0; pass < 4; ++pass)
        {
            for (int sample = 0; sample < blockSize; ++sample)
            {
                const auto input = 0.7f * std::sin(2.0f * pi * 11000.0f * static_cast<float>(sample + pass * blockSize) / static_cast<float>(sampleRate));
                outputs[static_cast<size_t>(mode)][static_cast<size_t>(sample)] = input;
                right[static_cast<size_t>(sample)] = input;
            }

            float* channels[] { outputs[static_cast<size_t>(mode)].data(), right.data() };
            engine.process(channels, 2, blockSize);
        }
    }

    return check(finiteBuffer(outputs[0]) && finiteBuffer(outputs[1]) && finiteBuffer(outputs[2]), "all oversampling modes stay finite")
        && check(peakMagnitude(outputs[1]) < 0.95f && peakMagnitude(outputs[2]) < 0.95f, "oversampled character remains bounded")
        && check(residualRms(outputs[0], outputs[1]) > 0.001f && residualRms(outputs[1], outputs[2]) > 0.0001f, "2x and 4x oversampling modes alter nonlinear processing");
}

float processorReductionForInputDrive(float inputDriveDb)
{
    constexpr int blockSize = 512;
    CompressorAudioProcessor processor;
    setFetProcessorDefaults(processor);
    setProcessorParameter(processor, "input", inputDriveDb);
    processor.prepareToPlay(48000.0, blockSize);

    juce::AudioBuffer<float> buffer(2, blockSize);
    processRepeatedProcessorBlocks(processor, buffer, 0.20f, 80);
    return processor.getGainReductionDb();
}

bool testProcessorInputDrivesCompression()
{
    const auto lowDriveReductionDb = processorReductionForInputDrive(-12.0f);
    const auto highDriveReductionDb = processorReductionForInputDrive(12.0f);

    return check(highDriveReductionDb > lowDriveReductionDb + 6.0f, "processor input control drives more feedback compression");
}

bool testProcessorBypassReturnsDrySignal()
{
    constexpr int blockSize = 512;
    constexpr auto amplitude = 0.25f;
    CompressorAudioProcessor processor;
    setFetProcessorDefaults(processor);
    setProcessorParameter(processor, "bypass", 1.0f);
    processor.prepareToPlay(48000.0, blockSize);

    juce::AudioBuffer<float> buffer(2, blockSize);
    processRepeatedProcessorBlocks(processor, buffer, amplitude, 100);

    auto matchesDrySignal = true;
    for (int sample = 0; sample < blockSize; ++sample)
    {
        const auto expected = sample % 2 == 0 ? amplitude : -amplitude;
        matchesDrySignal = matchesDrySignal && approximatelyEqual(buffer.getSample(0, sample), expected, 0.0005f)
                                      && approximatelyEqual(buffer.getSample(1, sample), expected, 0.0005f);
    }

    return check(matchesDrySignal, "processor bypass returns the dry signal when trims are neutral");
}

float processorPeakForMix(float mixPercent)
{
    constexpr int blockSize = 512;
    CompressorAudioProcessor processor;
    setFetProcessorDefaults(processor);
    setProcessorParameter(processor, "input", 12.0f);
    setProcessorParameter(processor, "mix", mixPercent);
    processor.prepareToPlay(48000.0, blockSize);

    juce::AudioBuffer<float> buffer(2, blockSize);
    processRepeatedProcessorBlocks(processor, buffer, 0.20f, 90);
    return peakMagnitude(buffer);
}

bool testProcessorMixBlendsDryAndWet()
{
    const auto dryPeak = processorPeakForMix(0.0f);
    const auto halfPeak = processorPeakForMix(50.0f);
    const auto wetPeak = processorPeakForMix(100.0f);

    return check(wetPeak < halfPeak && halfPeak < dryPeak, "processor mix blends between compressed wet and driven dry paths");
}

float processorReductionForOutputSettings(float makeupDb, float outputDb)
{
    constexpr int blockSize = 512;
    CompressorAudioProcessor processor;
    setFetProcessorDefaults(processor);
    setProcessorParameter(processor, "input", 12.0f);
    setProcessorParameter(processor, "makeup", makeupDb);
    setProcessorParameter(processor, "output", outputDb);
    processor.prepareToPlay(48000.0, blockSize);

    juce::AudioBuffer<float> buffer(2, blockSize);
    processRepeatedProcessorBlocks(processor, buffer, 0.20f, 90);
    return processor.getGainReductionDb();
}

bool testProcessorMakeupAndOutputDoNotDriveDetector()
{
    const auto neutralReductionDb = processorReductionForOutputSettings(0.0f, 0.0f);
    const auto hotMakeupReductionDb = processorReductionForOutputSettings(12.0f, 0.0f);
    const auto hotOutputReductionDb = processorReductionForOutputSettings(0.0f, 12.0f);

    return check(std::abs(neutralReductionDb - hotMakeupReductionDb) < 0.35f
                     && std::abs(neutralReductionDb - hotOutputReductionDb) < 0.35f,
                 "processor makeup and output gain do not drive the feedback detector");
}

bool testProcessorMetersPublishUsefulValues()
{
    constexpr int blockSize = 512;
    CompressorAudioProcessor processor;
    setFetProcessorDefaults(processor);
    setProcessorParameter(processor, "input", 12.0f);
    setProcessorParameter(processor, "output", -3.0f);
    processor.prepareToPlay(48000.0, blockSize);

    juce::AudioBuffer<float> buffer(2, blockSize);
    processRepeatedProcessorBlocks(processor, buffer, 0.20f, 90);

    return check(processor.getGainReductionDb() > 3.0f, "processor publishes gain reduction meter value")
        && check(std::isfinite(processor.getOutputLevelDb()) && processor.getOutputLevelDb() > -80.0f, "processor publishes output level meter value")
        && check(std::isfinite(processor.getMeterGainChangeDb()), "processor publishes signed gain-change meter value");
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    bool passed = true;
    for (const auto sampleRate : { 44100.0, 48000.0, 96000.0 })
        for (const auto blockSize : { 32, 64, 128, 256, 512, 1024 })
            passed = testProcessingMatrix(sampleRate, blockSize) && passed;

    passed = testCompressionBehavior() && passed;
    passed = testNoCompressionBelowThresholdAndUnityRatio() && passed;
    passed = testSoftKneeAndSidechainHighPass() && passed;
    passed = testGainReductionMeterBallistics() && passed;
    passed = testHybridDetectorModes() && passed;
    passed = testProgramDependentRelease() && passed;
    passed = testFetRatioExtremes() && passed;
    passed = testOutputGainDoesNotDriveFeedbackSidechain() && passed;
    passed = testCharacterStageAndOversampling() && passed;
    passed = testOversamplingModesAreFunctional() && passed;
    passed = testProcessorInputDrivesCompression() && passed;
    passed = testProcessorBypassReturnsDrySignal() && passed;
    passed = testProcessorMixBlendsDryAndWet() && passed;
    passed = testProcessorMakeupAndOutputDoNotDriveDetector() && passed;
    passed = testProcessorMetersPublishUsefulValues() && passed;

    if (!passed)
        return 1;

    std::cout << "Compressor DSP and processor tests passed\n";
}
