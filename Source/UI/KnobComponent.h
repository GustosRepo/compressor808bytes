#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace compressor808bytes
{
class KnobComponent final : public juce::Component
{
public:
    enum class Size
    {
        Small,
        Medium,
        Large
    };

    KnobComponent(const juce::String& name, const juce::String& suffix);
    void paint(juce::Graphics&) override;
    void resized() override;
    void setControlSize(Size newSize);
    juce::Slider slider;

private:
    void updateValueText();

    Size controlSize { Size::Medium };
    juce::String unit;
    juce::Label label;
    juce::Label value;
};
} // namespace compressor808bytes
