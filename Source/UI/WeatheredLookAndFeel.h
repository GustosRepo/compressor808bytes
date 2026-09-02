#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace compressor808bytes
{
class WeatheredLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    WeatheredLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height, float sliderPosition,
                          float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;
};
} // namespace compressor808bytes