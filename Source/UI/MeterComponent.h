#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <atomic>

namespace compressor808bytes
{
class MeterComponent final : public juce::Component
{
public:
    MeterComponent(const juce::String& name, const std::atomic<float>& source, bool isGainReduction = false);
    void paint(juce::Graphics& graphics) override;

private:
    juce::String name;
    const std::atomic<float>& source;
    bool isGainReduction;
};
} // namespace compressor808bytes