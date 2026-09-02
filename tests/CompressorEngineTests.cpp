#include "DSP/CompressorEngine.h"
#include "DSP/MeterBallistics.h"

#include <cmath>
#include <iostream>
#include <string_view>
#include <vector>

using namespace compressor808bytes;

namespace
{
constexpr float decibelTolerance = 0.15f;

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

    return check(approximatelyEqual(compressedGainDb, -15.0f), "4:1 ratio produces expected hard-knee reduction")
        && check(approximatelyEqual(engine.getGainReductionDb(), 15.0f), "gain-reduction meter reports dB reduction");
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
} // namespace

int main()
{
    bool passed = true;
    for (const auto sampleRate : { 44100.0, 48000.0, 96000.0 })
        for (const auto blockSize : { 32, 64, 128, 256, 512, 1024 })
            passed = testProcessingMatrix(sampleRate, blockSize) && passed;

    passed = testCompressionBehavior() && passed;
    passed = testNoCompressionBelowThresholdAndUnityRatio() && passed;
    passed = testSoftKneeAndSidechainHighPass() && passed;
    passed = testGainReductionMeterBallistics() && passed;

    if (!passed)
        return 1;

    std::cout << "Compressor DSP tests passed\n";
}
