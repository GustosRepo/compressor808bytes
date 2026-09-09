#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <atomic>

namespace compressor808bytes
{
enum class MeterMode
{
    GainReduction,
    OutputPlus4,
    OutputPlus8
};

class MeterComponent final : public juce::Component
{
public:
    MeterComponent(const juce::String& name, const std::atomic<float>& source, bool isGainReduction = false);
    MeterComponent(const juce::String& name, const std::atomic<float>& inputSource, const std::atomic<float>& outputSource,
                   const std::atomic<float>& gainReductionSource);
    void paint(juce::Graphics& graphics) override;
    void setMeterMode(MeterMode newMode) noexcept;

private:
    juce::String name;
    const std::atomic<float>* source { nullptr };
    const std::atomic<float>* outputSource { nullptr };
    const std::atomic<float>* gainReductionSource { nullptr };
    MeterMode meterMode { MeterMode::GainReduction };
    bool isLargeMeter { false };
};
} // namespace compressor808bytes
