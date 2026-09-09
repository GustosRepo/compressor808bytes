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

struct ScaleStrip
{
    juce::String left;
    juce::String centre;
    juce::String right;
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

    if (unit == "ms" && maximum <= 1.0f)
        return { { 0.02f, "0.02" }, { 0.2f, "0.2" }, { 0.5f, "0.5" }, { 0.8f, "0.8" } };

    if (unit == "ms")
        return { { 50.0f, "50" }, { 250.0f, "250" }, { 600.0f, "600" }, { 1100.0f, "1.1K" } };

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

ScaleStrip scaleStripForSlider(const juce::Slider& slider, const juce::String& unit, const juce::String& controlName)
{
    const auto minimum = static_cast<float>(slider.getMinimum());
    const auto maximum = static_cast<float>(slider.getMaximum());

    if (unit == "dB" && closeTo(minimum, -24.0f) && closeTo(maximum, 24.0f))
        return { "-24", "0", "+24" };

    if (unit == "ms" && maximum <= 1.0f)
        return { {}, "SLOW - FAST", {} };

    if (unit == "ms")
        return { {}, "FAST - SLOW", {} };

    if (unit == "%")
        return { {}, "0 - 100", {} };

    if (unit == "Hz")
        return { {}, "20 - 500", {} };

    if (unit == "dB" && controlName == "KNEE")
        return { {}, "0 - " + juce::String(juce::roundToInt(maximum)), {} };

    return {};
}

} // namespace

KnobComponent::KnobComponent(const juce::String& name, const juce::String& suffix)
    : controlName(name.toUpperCase()), unit(suffix)
{
    label.setText(controlName, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 0.75f, juce::MathConstants<float>::pi * 2.25f, true);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setTextValueSuffix(suffix == ":1" ? suffix : " " + suffix);
    slider.setNumDecimalPlacesToDisplay(suffix == "Hz" ? 0 : 1);
    slider.onValueChange = [this] { updateValueText(); };

    value.setJustificationType(juce::Justification::centred);
    value.setInterceptsMouseClicks(false, false);
    label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(33, 37, 41));

    value.setColour(juce::Label::textColourId, juce::Colour::fromRGB(229, 236, 233));
    value.setColour(juce::Label::backgroundColourId, juce::Colour::fromRGB(16, 19, 22));
    value.setColour(juce::Label::outlineColourId, juce::Colour::fromRGB(72, 82, 88));
    value.setFont(juce::FontOptions(18.0f).withStyle("Bold"));

    addAndMakeVisible(label);
    addAndMakeVisible(slider);
    addAndMakeVisible(value);
    updateValueText();
}

void KnobComponent::paint(juce::Graphics& graphics)
{
    const auto marks = marksForSlider(slider, unit);
    const auto knobArea = slider.getBounds().toFloat();
    const auto side = std::min(knobArea.getWidth(), knobArea.getHeight());

    if (!marks.empty() && side >= 44.0f)
    {
        const auto centre = knobArea.withSizeKeepingCentre(side, side).getCentre();
        const auto radius = side * 0.39f;
        const auto startAngle = juce::MathConstants<float>::pi * 0.75f;
        const auto endAngle = juce::MathConstants<float>::pi * 2.25f;

        graphics.setColour(juce::Colour::fromRGB(67, 75, 80).withAlpha(0.76f));

        for (const auto& mark : marks)
        {
            if (mark.value < slider.getMinimum() || mark.value > slider.getMaximum())
                continue;

            const auto proportion = static_cast<float>(slider.valueToProportionOfLength(mark.value));
            const auto angle = startAngle + proportion * (endAngle - startAngle);
            const auto tickStart = centre + juce::Point<float>(std::cos(angle), std::sin(angle)) * (radius + 2.0f);
            const auto tickEnd = centre + juce::Point<float>(std::cos(angle), std::sin(angle)) * (radius + (side > 74.0f ? 8.0f : 5.0f));

            graphics.drawLine(tickStart.x, tickStart.y, tickEnd.x, tickEnd.y, closeTo(mark.value, 0.0f) ? 1.8f : 1.1f);
        }
    }

    const auto scaleStrip = scaleStripForSlider(slider, unit, controlName);
    if (scaleStrip.left.isEmpty() && scaleStrip.centre.isEmpty() && scaleStrip.right.isEmpty())
        return;

    const auto valueBounds = value.getBounds().toFloat();
    const auto stripHeight = juce::jlimit(10.0f, 15.0f, side * 0.11f);
    const auto stripGap = juce::jmax(4.0f, side * 0.035f);
    const auto usesCentredRange = scaleStrip.left.isEmpty() && scaleStrip.right.isEmpty();
    auto stripArea = valueBounds.withY(valueBounds.getY() - stripHeight - stripGap)
                                    .withHeight(stripHeight)
                                    .expanded(usesCentredRange ? 4.0f : side >= 112.0f ? 22.0f : 12.0f, 0.0f);
    stripArea = stripArea.getIntersection(getLocalBounds().toFloat().reduced(3.0f));

    graphics.setFont(juce::FontOptions(usesCentredRange ? juce::jlimit(7.4f, 8.8f, side * 0.078f)
                                                        : juce::jlimit(7.5f, 10.5f, side * 0.085f))
                         .withStyle("Bold"));
    graphics.setColour(juce::Colour::fromRGB(58, 66, 71).withAlpha(usesCentredRange ? 0.74f : 0.88f));

    if (usesCentredRange)
    {
        graphics.drawFittedText(scaleStrip.centre, stripArea.toNearestInt(), juce::Justification::centred, 1, 0.86f);
        return;
    }

    const auto third = stripArea.getWidth() / 3.0f;
    auto leftArea = stripArea.removeFromLeft(third);
    auto centreArea = stripArea.removeFromLeft(third);
    auto rightArea = stripArea;

    graphics.drawFittedText(scaleStrip.left, leftArea.toNearestInt(), juce::Justification::centredLeft, 1);
    graphics.drawFittedText(scaleStrip.centre, centreArea.toNearestInt(), juce::Justification::centred, 1);
    graphics.drawFittedText(scaleStrip.right, rightArea.toNearestInt(), juce::Justification::centredRight, 1);
}

void KnobComponent::resized()
{
    auto bounds = getLocalBounds();
    const auto isLarge = controlSize == Size::Large;
    const auto isSmall = controlSize == Size::Small;
    const auto compactScale = juce::jlimit(0.76f, 1.0f, static_cast<float>(getWidth()) / (isSmall ? 112.0f : isLarge ? 184.0f : 138.0f));
    label.setFont(juce::FontOptions((isLarge ? 15.0f : isSmall ? 12.5f : 14.0f) * compactScale).withStyle("Bold"));
    value.setFont(juce::FontOptions((isLarge ? 20.0f : isSmall ? 15.5f : 18.0f) * compactScale).withStyle("Bold"));

    label.setBounds(bounds.removeFromTop(juce::roundToInt((isSmall ? 21.0f : 24.0f) * compactScale)));
    bounds.removeFromTop(juce::roundToInt((isSmall ? 6.0f : 10.0f) * compactScale));

    auto readoutArea = bounds.removeFromBottom(juce::roundToInt((isSmall ? 24.0f : 28.0f) * compactScale));
    const auto readoutWidth = juce::roundToInt((isLarge ? 116.0f : isSmall ? 84.0f : 98.0f) * compactScale);
    value.setBounds(readoutArea.withSizeKeepingCentre(juce::jmin(readoutWidth, readoutArea.getWidth() - 8), juce::roundToInt((isSmall ? 22.0f : 25.0f) * compactScale)));
    bounds.removeFromBottom(juce::roundToInt((isSmall ? 8.0f : 10.0f) * compactScale));

    const auto maximumSide = juce::roundToInt((isLarge ? 164.0f : isSmall ? 94.0f : 128.0f) * compactScale);
    const auto horizontalPadding = juce::roundToInt((isLarge ? 54.0f : isSmall ? 28.0f : 48.0f) * compactScale);
    const auto sliderSide = juce::jmax(36, juce::jmin(juce::jmin(bounds.getWidth() - horizontalPadding, bounds.getHeight()), maximumSide));
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
