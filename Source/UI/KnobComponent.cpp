#include "KnobComponent.h"

#include <cmath>
#include <vector>

namespace compressor808bytes
{
namespace
{
struct KnobMark
{
    float value;
    juce::String label;
};

bool closeTo(float first, float second) noexcept
{
    return std::abs(first - second) < 0.01f;
}

std::vector<KnobMark> marksForSlider(const juce::Slider& slider, const juce::String& unit)
{
    const auto minimum = static_cast<float>(slider.getMinimum());
    const auto maximum = static_cast<float>(slider.getMaximum());

    if (unit == ":1")
        return { { 1.0f, "1" }, { 2.0f, "2" }, { 4.0f, "4" }, { 8.0f, "8" }, { 20.0f, "20" } };

    if (unit == "%")
        return { { 0.0f, "0" }, { 50.0f, "50" }, { 100.0f, "100" } };

    if (unit == "Hz")
        return { { 20.0f, "20" }, { 60.0f, "60" }, { 120.0f, "120" }, { 500.0f, "500" } };

    if (unit == "ms" && closeTo(minimum, 0.1f) && closeTo(maximum, 100.0f))
        return { { 0.1f, "0.1" }, { 1.0f, "1" }, { 10.0f, "10" }, { 100.0f, "100" } };

    if (unit == "ms")
        return { { 10.0f, "10" }, { 100.0f, "100" }, { 500.0f, "500" }, { 2000.0f, "2K" } };

    if (unit == "dB" && closeTo(minimum, -60.0f))
        return { { -60.0f, "-60" }, { -45.0f, "-45" }, { -30.0f, "-30" }, { -15.0f, "-15" }, { 0.0f, "0" } };

    if (unit == "dB" && closeTo(minimum, -24.0f) && closeTo(maximum, 24.0f))
        return { { -24.0f, "-24" }, { -12.0f, "-12" }, { 0.0f, "0" }, { 12.0f, "+12" }, { 24.0f, "+24" } };

    if (unit == "dB" && closeTo(minimum, -24.0f) && closeTo(maximum, 12.0f))
        return { { -24.0f, "-24" }, { -12.0f, "-12" }, { 0.0f, "0" }, { 12.0f, "+12" } };

    if (unit == "dB" && closeTo(minimum, -12.0f) && closeTo(maximum, 24.0f))
        return { { -12.0f, "-12" }, { 0.0f, "0" }, { 12.0f, "+12" }, { 24.0f, "+24" } };

    if (unit == "dB")
        return { { 0.0f, "0" }, { 6.0f, "6" }, { 12.0f, "12" }, { 24.0f, "24" } };

    return {};
}
} // namespace

KnobComponent::KnobComponent(const juce::String& name, const juce::String& suffix)
    : unit(suffix)
{
    label.setText(name.toUpperCase(), juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.20f, juce::MathConstants<float>::pi * 2.80f, true);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setTextValueSuffix(suffix == ":1" ? suffix : " " + suffix);
    slider.setNumDecimalPlacesToDisplay(suffix == "Hz" ? 0 : 1);
    slider.onValueChange = [this] { updateValueText(); };

    value.setJustificationType(juce::Justification::centred);
    value.setInterceptsMouseClicks(false, false);
    value.setColour(juce::Label::textColourId, juce::Colour::fromRGB(184, 164, 123));
    value.setColour(juce::Label::backgroundColourId, juce::Colour::fromRGB(8, 8, 7));
    value.setColour(juce::Label::outlineColourId, juce::Colour::fromRGB(47, 40, 31));
    value.setFont(juce::FontOptions(18.0f).withStyle("Bold"));

    addAndMakeVisible(label);
    addAndMakeVisible(slider);
    addAndMakeVisible(value);
    updateValueText();
}

void KnobComponent::paint(juce::Graphics& graphics)
{
    const auto marks = marksForSlider(slider, unit);
    if (marks.empty())
        return;

    const auto knobArea = slider.getBounds().toFloat();
    const auto side = std::min(knobArea.getWidth(), knobArea.getHeight());
    const auto centre = knobArea.withSizeKeepingCentre(side, side).getCentre();
    const auto radius = side * 0.39f;
    const auto startAngle = juce::MathConstants<float>::pi * 1.20f;
    const auto endAngle = juce::MathConstants<float>::pi * 2.80f;

    graphics.setColour(juce::Colour::fromRGB(28, 25, 20));
    graphics.setFont(juce::FontOptions(side > 104.0f ? 12.0f : 10.0f));

    for (const auto& mark : marks)
    {
        if (mark.value < slider.getMinimum() || mark.value > slider.getMaximum())
            continue;

        const auto proportion = static_cast<float>(slider.valueToProportionOfLength(mark.value));
        const auto angle = startAngle + proportion * (endAngle - startAngle);
        const auto tickStart = centre + juce::Point<float>(std::cos(angle), std::sin(angle)) * (radius + 2.0f);
        const auto tickEnd = centre + juce::Point<float>(std::cos(angle), std::sin(angle)) * (radius + 9.0f);
        const auto position = centre + juce::Point<float>(std::cos(angle), std::sin(angle)) * (radius + 21.0f);

        graphics.drawLine(tickStart.x, tickStart.y, tickEnd.x, tickEnd.y, mark.value == 0.0f ? 1.8f : 1.1f);
        graphics.drawText(mark.label, juce::Rectangle<float>(position.x - 20.0f, position.y - 8.0f, 40.0f, 16.0f), juce::Justification::centred);
    }
}

void KnobComponent::resized()
{
    auto bounds = getLocalBounds();
    const auto isLarge = controlSize == Size::Large;
    const auto isSmall = controlSize == Size::Small;
    label.setFont(juce::FontOptions(isLarge ? 15.0f : isSmall ? 12.5f : 14.0f).withStyle("Bold"));
    value.setFont(juce::FontOptions(isLarge ? 20.0f : isSmall ? 15.5f : 18.0f).withStyle("Bold"));

    label.setBounds(bounds.removeFromTop(isSmall ? 21 : 24));
    bounds.removeFromTop(isSmall ? 6 : 10);

    auto readoutArea = bounds.removeFromBottom(isSmall ? 24 : 28);
    const auto readoutWidth = isLarge ? 116 : isSmall ? 84 : 98;
    value.setBounds(readoutArea.withSizeKeepingCentre(juce::jmin(readoutWidth, readoutArea.getWidth() - 10), isSmall ? 22 : 25));
    bounds.removeFromBottom(isSmall ? 8 : 10);

    const auto maximumSide = isLarge ? 164 : isSmall ? 94 : 128;
    const auto horizontalPadding = isLarge ? 54 : isSmall ? 34 : 48;
    const auto sliderSide = juce::jmin(juce::jmin(bounds.getWidth() - horizontalPadding, bounds.getHeight()), maximumSide);
    slider.setBounds(bounds.withSizeKeepingCentre(sliderSide, sliderSide));
}

void KnobComponent::setControlSize(Size newSize)
{
    if (controlSize == newSize)
        return;

    controlSize = newSize;
    resized();
    repaint();
}

void KnobComponent::updateValueText()
{
    const auto rawValue = static_cast<float>(slider.getValue());
    juce::String suffix = unit == ":1" ? ":1" : " " + unit;

    if (unit == "Hz")
        value.setText(juce::String(juce::roundToInt(rawValue)) + suffix, juce::dontSendNotification);
    else if (unit == "%")
        value.setText(juce::String(rawValue, 1) + suffix, juce::dontSendNotification);
    else
        value.setText(juce::String(rawValue, 1) + suffix, juce::dontSendNotification);
}
} // namespace compressor808bytes
