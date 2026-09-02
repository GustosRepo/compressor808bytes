#pragma once

namespace compressor808bytes
{
class EnvelopeDetector
{
public:
    void prepare(double sampleRate);
    void reset();
    void setTimes(float attackMs, float releaseMs);
    [[nodiscard]] float processPower(float power) noexcept;

private:
    double sampleRateHz { 44100.0 };
    float attackCoefficient { 0.0f };
    float releaseCoefficient { 0.0f };
    float envelopePower { 0.0f };
};
} // namespace compressor808bytes