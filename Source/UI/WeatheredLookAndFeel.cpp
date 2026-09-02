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
    setColour(juce::ComboBox::textColourId, juce::Colour::fromRGB(184, 164, 123));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour::fromRGB(8, 8, 7));
    setColour(juce::ComboBox::outlineColourId, juce::Colour::fromRGB(47, 40, 31));
    setColour(juce::ComboBox::arrowColourId, juce::Colour::fromRGB(184, 164, 123));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour::fromRGB(13, 12, 10));
    setColour(juce::PopupMenu::textColourId, juce::Colour::fromRGB(205, 193, 166));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour::fromRGB(85, 70, 48));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour::fromRGB(232, 219, 186));
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
    const auto visualScale = juce::jlimit(0.56f, 1.0f, side / 128.0f);

    graphics.setColour(juce::Colour::fromRGB(10, 10, 9).withAlpha(0.52f));
    graphics.fillEllipse(bounds.expanded(6.0f * visualScale).translated(3.0f * visualScale, 5.0f * visualScale));
    graphics.setColour(juce::Colour::fromRGB(31, 27, 22));
    graphics.fillEllipse(bounds.expanded(4.0f * visualScale));
    graphics.setColour(juce::Colour::fromRGB(93, 82, 63));
    graphics.drawEllipse(bounds.expanded(4.0f * visualScale), 1.5f * visualScale);
    graphics.setColour(juce::Colour::fromRGB(12, 12, 11));
    graphics.fillEllipse(bounds);
    graphics.setColour(juce::Colour::fromRGB(61, 55, 45));
    graphics.drawEllipse(bounds, 2.0f * visualScale);

    auto knob = bounds.reduced(radius * 0.13f);
    graphics.setColour(knobBlack);
    graphics.fillEllipse(knob);
    graphics.setColour(juce::Colour::fromRGB(8, 8, 7));
    graphics.drawEllipse(knob, 3.0f * visualScale);
    graphics.setColour(juce::Colour::fromRGB(38, 35, 30));
    graphics.fillEllipse(knob.reduced(radius * 0.34f).translated(-radius * 0.14f, -radius * 0.16f));
    graphics.setColour(juce::Colour::fromRGB(49, 44, 35));
    graphics.drawEllipse(knob.reduced(radius * 0.12f), 1.2f * visualScale);

    for (int scuff = 0; scuff < 16; ++scuff)
    {
        const auto offset = -radius * 0.56f + static_cast<float>((scuff * 11) % 100) * radius * 0.011f;
        const auto yOffset = -radius * 0.42f + static_cast<float>((scuff * 17) % 82) * radius * 0.010f;
        const auto start = centre + juce::Point<float>(offset, yOffset);
        const auto end = start + juce::Point<float>(radius * (0.18f + static_cast<float>(scuff % 5) * 0.055f),
                                                    radius * (-0.08f + static_cast<float>(scuff % 4) * 0.035f));
        graphics.setColour(juce::Colours::white.withAlpha(scuff % 3 == 0 ? 0.085f : 0.045f));
        graphics.drawLine(start.x, start.y, end.x, end.y, (scuff % 6 == 0 ? 1.1f : 0.6f) * visualScale);
    }

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
    graphics.strokePath(grip, juce::PathStrokeType(2.0f * visualScale));

    const auto pointerStart = radius * 0.16f;
    const auto pointerEnd = radius * 0.72f;
    graphics.setColour(pointerPaint);
    graphics.drawLine(centre.x + std::cos(angle) * pointerStart, centre.y + std::sin(angle) * pointerStart,
                      centre.x + std::cos(angle) * pointerEnd, centre.y + std::sin(angle) * pointerEnd, 3.5f * visualScale);
    graphics.setColour(juce::Colour::fromRGB(46, 34, 22).withAlpha(0.52f));
    graphics.drawLine(centre.x + std::cos(angle) * (pointerEnd * 0.74f), centre.y + std::sin(angle) * (pointerEnd * 0.74f),
                      centre.x + std::cos(angle) * pointerEnd, centre.y + std::sin(angle) * pointerEnd, 1.2f * visualScale);
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

void WeatheredLookAndFeel::drawComboBox(juce::Graphics& graphics, int width, int height, bool isButtonDown,
                                        int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box)
{
    const auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)).reduced(0.5f);
    graphics.setColour(juce::Colour::fromRGB(3, 3, 3).withAlpha(0.42f));
    graphics.fillRoundedRectangle(bounds.translated(1.0f, 2.0f), 3.0f);
    graphics.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    graphics.fillRoundedRectangle(bounds, 3.0f);
    graphics.setColour(juce::Colours::white.withAlpha(0.05f));
    graphics.fillRoundedRectangle(bounds.withTrimmedBottom(bounds.getHeight() * 0.56f), 3.0f);
    graphics.setColour(juce::Colour::fromRGB(95, 58, 28).withAlpha(0.18f));
    graphics.drawLine(bounds.getX() + 5.0f, bounds.getBottom() - 5.0f, bounds.getRight() - 17.0f, bounds.getBottom() - 8.0f, 0.8f);
    graphics.drawLine(bounds.getX() + 13.0f, bounds.getY() + 7.0f, bounds.getRight() - 28.0f, bounds.getY() + 10.0f, 0.55f);
    graphics.setColour(isButtonDown ? juce::Colour::fromRGB(118, 96, 62) : box.findColour(juce::ComboBox::outlineColourId));
    graphics.drawRoundedRectangle(bounds, 3.0f, 1.2f);

    const auto buttonBounds = juce::Rectangle<float>(static_cast<float>(buttonX), static_cast<float>(buttonY),
                                                    static_cast<float>(buttonW), static_cast<float>(buttonH));
    juce::Path arrow;
    arrow.addTriangle(buttonBounds.getCentreX() - 4.5f, buttonBounds.getCentreY() - 2.0f,
                      buttonBounds.getCentreX() + 4.5f, buttonBounds.getCentreY() - 2.0f,
                      buttonBounds.getCentreX(), buttonBounds.getCentreY() + 4.0f);
    graphics.setColour(box.findColour(juce::ComboBox::arrowColourId));
    graphics.fillPath(arrow);
}

void WeatheredLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(8, 1, box.getWidth() - 24, box.getHeight() - 2);
    label.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
    label.setJustificationType(juce::Justification::centredLeft);
}
} // namespace compressor808bytes
