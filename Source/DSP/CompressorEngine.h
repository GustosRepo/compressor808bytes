#pragma once

#include "EnvelopeDetector.h"
#include "Filters.h"

#include <array>

namespace compressor808bytes
{
struct CompressorParameters
{
    float thresholdDb { -18.0f };
    float ratio { 4.0f };
    float attackMs { 10.0f };
    float releaseMs { 100.0f };
    float makeupDb { 0.0f };
    float kneeDb { 0.0f };
    float sidechainHighPassHz { 20.0f };
    int detectorMode { 0 };
    int character { 0 };
    int oversampling { 0 };
};

class CompressorEngine
{
public:
    void prepare(double sampleRate, int maximumBlockSize, int channelCount);
    void reset();
    void setParameters(const CompressorParameters& newParameters);
    void process(float* const* channels, int channelCount, int sampleCount) noexcept;
    [[nodiscard]] float getGainReductionDb() const noexcept { return gainReductionDb; }

private:
    [[nodiscard]] float calculateGainDb(float inputDb) const noexcept;
    [[nodiscard]] float processDetector(float linkedPower, float linkedPeak) noexcept;
    [[nodiscard]] float smoothGainDb(float targetGainDb) noexcept;
    [[nodiscard]] float processCharacter(float sample, int channel, float gainReductionDb) noexcept;
    [[nodiscard]] float shapeCharacterSample(float sample, float drive, float asymmetry) const noexcept;

    CompressorParameters parameters;
    std::array<SidechainHighPassFilter, 2> sidechainFilters;
    std::array<float, 2> previousCharacterInput {};
    std::array<float, 2> characterLowPassState {};
    std::array<bool, 2> characterStateInitialised {};
    double sampleRateHz { 44100.0 };
    float rmsEnvelopePower { 0.0f };
    float peakEnvelope { 0.0f };
    float smoothedGainDb { 0.0f };
    float gainReductionDb { 0.0f };
};
} // namespace compressor808bytes
