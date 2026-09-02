#pragma once

#include <cmath>

namespace compressor808bytes
{
class GainReductionMeterBallistics
{
public:
    void prepare(double newSampleRate) noexcept
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        sampleCoefficient = coefficientForSeconds(vuRiseTimeTo99PercentSeconds);
    }

    void reset(float initialGainReductionDb = 0.0f) noexcept
    {
        valueDb = initialGainReductionDb;
    }

    void processSample(float targetGainReductionDb) noexcept
    {
        valueDb = static_cast<float>(static_cast<double>(targetGainReductionDb) + static_cast<double>(valueDb - targetGainReductionDb) * sampleCoefficient);
    }

    [[nodiscard]] float getValueDb() const noexcept { return valueDb; }

    static constexpr double vuRiseTimeTo99PercentSeconds = 0.300;

private:
    [[nodiscard]] double coefficientForSeconds(double secondsTo99Percent) const noexcept
    {
        return std::exp(std::log(0.01) / (sampleRate * secondsTo99Percent));
    }

    double sampleRate { 44100.0 };
    double sampleCoefficient { coefficientForSeconds(vuRiseTimeTo99PercentSeconds) };
    float valueDb { 0.0f };
};
} // namespace compressor808bytes
