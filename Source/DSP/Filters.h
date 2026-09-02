#pragma once

namespace compressor808bytes
{
class SidechainHighPassFilter
{
public:
    void prepare(double sampleRate);
    void reset();
    void setCutoff(float frequencyHz);
    [[nodiscard]] float process(float sample) noexcept;

private:
    double sampleRateHz { 44100.0 };
    float coefficient { 0.0f };
    float previousInput { 0.0f };
    float previousOutput { 0.0f };
};
} // namespace compressor808bytes