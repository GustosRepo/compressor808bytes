#include "WeatheredLookAndFeel.h"

#include <cmath>

namespace compressor808bytes
{
namespace
{
const auto panelInk = juce::Colour::fromRGB(37, 31, 24);
const auto pointerPaint = juce::Colour::fromRGB(205, 193, 166);
const auto knobBlack = juce::Colour::fromRGB(18, 17, 15);
}

WeatheredLookAndFeel::WeatheredLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, juce::Colour::fromRGB(170, 151, 113));
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour::fromRGB(9, 9, 8));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colour::fromRGB(34, 29, 23));
    setColour(juce::Label::textColourId, panelInk);
}

void WeatheredLookAndFeel::drawRotarySlider(juce::Graphics& graphics, int x, int y, int width, int height,
                                            float sliderPosition, float startAngle, float endAngle, juce::Slider&)
{
    const auto rawBounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height));
    const auto side = std::min(rawBounds.getWidth(), rawBounds.getHeight());
    const auto bounds = rawBounds.withSizeKeepingCentre(side, side).reduced(16.0f);
    const auto radius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle = startAngle + sliderPosition * (endAngle - startAngle);

    graphics.setColour(juce::Colour::fromRGB(10, 10, 9).withAlpha(0.52f));
    graphics.fillEllipse(bounds.expanded(6.0f).translated(3.0f, 5.0f));
    graphics.setColour(juce::Colour::fromRGB(31, 27, 22));
    graphics.fillEllipse(bounds.expanded(4.0f));
    graphics.setColour(juce::Colour::fromRGB(93, 82, 63));
    graphics.drawEllipse(bounds.expanded(4.0f), 1.5f);
    graphics.setColour(juce::Colour::fromRGB(12, 12, 11));
    graphics.fillEllipse(bounds);
    graphics.setColour(juce::Colour::fromRGB(61, 55, 45));
    graphics.drawEllipse(bounds, 2.0f);

    auto knob = bounds.reduced(radius * 0.13f);
    graphics.setColour(knobBlack);
    graphics.fillEllipse(knob);
    graphics.setColour(juce::Colour::fromRGB(8, 8, 7));
    graphics.drawEllipse(knob, 3.0f);
    graphics.setColour(juce::Colour::fromRGB(38, 35, 30));
    graphics.fillEllipse(knob.reduced(radius * 0.34f).translated(-radius * 0.14f, -radius * 0.16f));
    graphics.setColour(juce::Colour::fromRGB(49, 44, 35));
    graphics.drawEllipse(knob.reduced(radius * 0.12f), 1.2f);

    juce::Path grip;
    for (int tooth = 0; tooth < 12; ++tooth)
    {
        const auto gripAngle = static_cast<float>(tooth) * juce::MathConstants<float>::twoPi / 12.0f;
        const auto inner = radius * 0.60f;
        const auto outer = radius * 0.76f;
        grip.startNewSubPath(centre.x + std::cos(gripAngle) * inner, centre.y + std::sin(gripAngle) * inner);
        grip.lineTo(centre.x + std::cos(gripAngle) * outer, centre.y + std::sin(gripAngle) * outer);
    }
    graphics.setColour(juce::Colour::fromRGB(75, 68, 55).withAlpha(0.55f));
    graphics.strokePath(grip, juce::PathStrokeType(2.0f));

    const auto pointerStart = radius * 0.16f;
    const auto pointerEnd = radius * 0.72f;
    graphics.setColour(pointerPaint);
    graphics.drawLine(centre.x + std::cos(angle) * pointerStart, centre.y + std::sin(angle) * pointerStart,
                      centre.x + std::cos(angle) * pointerEnd, centre.y + std::sin(angle) * pointerEnd, 3.5f);
}

void WeatheredLookAndFeel::drawToggleButton(juce::Graphics& graphics, juce::ToggleButton& button, bool, bool)
{
    auto bounds = button.getLocalBounds().toFloat();
    const auto lampSize = std::min(bounds.getWidth() * 0.54f, bounds.getHeight() * 0.56f);
    const auto switchBounds = juce::Rectangle<float>(lampSize, lampSize).withCentre({ bounds.getCentreX(), bounds.getY() + lampSize * 0.58f });
    graphics.setColour(juce::Colour::fromRGB(10, 10, 9).withAlpha(0.5f));
    graphics.fillEllipse(switchBounds.expanded(6.0f).translated(2.0f, 4.0f));
    graphics.setColour(juce::Colour::fromRGB(13, 12, 10));
    graphics.fillEllipse(switchBounds.expanded(4.0f));
    graphics.setColour(juce::Colour::fromRGB(93, 82, 63));
    graphics.drawEllipse(switchBounds.expanded(4.0f), 2.0f);
    const auto lit = button.getToggleState();
    graphics.setColour(lit ? juce::Colour::fromRGB(255, 159, 37) : juce::Colour::fromRGB(106, 72, 28));
    graphics.fillEllipse(switchBounds.reduced(5.0f));
    graphics.setColour(juce::Colours::white.withAlpha(lit ? 0.55f : 0.16f));
    graphics.fillEllipse(switchBounds.reduced(lampSize * 0.34f).translated(-lampSize * 0.1f, -lampSize * 0.13f));
    graphics.setColour(panelInk);
    graphics.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
    graphics.drawText(button.getButtonText().toUpperCase(), bounds.withTrimmedTop(lampSize + 12.0f).toNearestInt(), juce::Justification::centred);
}
} // namespace compressor808bytes
