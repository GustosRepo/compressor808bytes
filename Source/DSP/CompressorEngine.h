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

    CompressorParameters parameters;
    EnvelopeDetector detector;
    std::array<SidechainHighPassFilter, 2> sidechainFilters;
    float gainReductionDb { 0.0f };
};
} // namespace compressor808bytes