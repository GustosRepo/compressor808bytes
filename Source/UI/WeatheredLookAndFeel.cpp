#include "WeatheredLookAndFeel.h"

#include <cmath>

namespace compressor808bytes
{
namespace
{
const auto panelText = juce::Colour::fromRGB(31, 35, 39);
const auto displayText = juce::Colour::fromRGB(227, 235, 232);
const auto accentOrange = juce::Colour::fromRGB(242, 102, 53);
const auto accentYellow = juce::Colour::fromRGB(237, 190, 72);
const auto controlEdge = juce::Colour::fromRGB(75, 83, 89);
} // namespace

WeatheredLookAndFeel::WeatheredLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, displayText);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour::fromRGB(15, 18, 20));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colour::fromRGB(54, 61, 66));
    setColour(juce::Label::textColourId, panelText);
    setColour(juce::ComboBox::textColourId, displayText);
    setColour(juce::ComboBox::backgroundColourId, juce::Colour::fromRGB(21, 24, 27));
    setColour(juce::ComboBox::outlineColourId, juce::Colour::fromRGB(73, 81, 86));
    setColour(juce::ComboBox::arrowColourId, accentYellow);
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour::fromRGB(18, 21, 24));
    setColour(juce::PopupMenu::textColourId, displayText);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, accentOrange);
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
}

void WeatheredLookAndFeel::drawRotarySlider(juce::Graphics& graphics, int x, int y, int width, int height,
                                            float sliderPosition, float startAngle, float endAngle, juce::Slider&)
{
    const auto rawBounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height));
    const auto side = std::min(rawBounds.getWidth(), rawBounds.getHeight());
    const auto visualScale = juce::jlimit(0.58f, 1.0f, side / 132.0f);
    const auto bounds = rawBounds.withSizeKeepingCentre(side, side).reduced(13.0f * visualScale);
    const auto radius = bounds.getWidth() * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle = startAngle + sliderPosition * (endAngle - startAngle);

    graphics.setColour(juce::Colour::fromRGB(10, 12, 14).withAlpha(0.18f));
    graphics.fillEllipse(bounds.translated(0.0f, 5.0f * visualScale).expanded(5.0f * visualScale));

    juce::Path track;
    track.addCentredArc(centre.x, centre.y, radius * 0.96f, radius * 0.96f, 0.0f, startAngle, endAngle, true);
    graphics.setColour(juce::Colour::fromRGB(190, 198, 200));
    graphics.strokePath(track, juce::PathStrokeType(3.0f * visualScale, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path progress;
    progress.addCentredArc(centre.x, centre.y, radius * 0.96f, radius * 0.96f, 0.0f, startAngle, angle, true);
    graphics.setColour(accentOrange);
    graphics.strokePath(progress, juce::PathStrokeType(4.0f * visualScale, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    graphics.setColour(juce::Colour::fromRGB(226, 231, 230));
    graphics.fillEllipse(bounds.expanded(3.0f * visualScale));
    graphics.setColour(juce::Colour::fromRGB(152, 161, 164));
    graphics.drawEllipse(bounds.expanded(3.0f * visualScale), 1.0f * visualScale);

    auto knob = bounds.reduced(radius * 0.15f);
    juce::ColourGradient knobGradient(juce::Colour::fromRGB(47, 53, 58), knob.getX(), knob.getY(),
                                      juce::Colour::fromRGB(15, 17, 20), knob.getRight(), knob.getBottom(), false);
    graphics.setGradientFill(knobGradient);
    graphics.fillEllipse(knob);
    graphics.setColour(controlEdge);
    graphics.drawEllipse(knob, 1.5f * visualScale);

    graphics.setColour(juce::Colours::white.withAlpha(0.10f));
    graphics.fillEllipse(knob.reduced(radius * 0.18f).withTrimmedBottom(radius * 0.38f));

    const auto pointerStart = radius * 0.18f;
    const auto pointerEnd = radius * 0.66f;
    graphics.setColour(accentYellow);
    graphics.drawLine(centre.x + std::cos(angle) * pointerStart, centre.y + std::sin(angle) * pointerStart,
                      centre.x + std::cos(angle) * pointerEnd, centre.y + std::sin(angle) * pointerEnd, 3.0f * visualScale);

    graphics.setColour(juce::Colour::fromRGB(12, 14, 16));
    graphics.fillEllipse(juce::Rectangle<float>(radius * 0.12f, radius * 0.12f).withCentre(centre));
}

void WeatheredLookAndFeel::drawToggleButton(juce::Graphics& graphics, juce::ToggleButton& button, bool highlighted, bool down)
{
    const auto bounds = button.getLocalBounds().toFloat();
    const auto lit = button.getToggleState();
    const auto scale = juce::jlimit(0.58f, 1.0f, std::min(bounds.getWidth() / 92.0f, bounds.getHeight() / 92.0f));
    const auto switchSize = std::min(bounds.getWidth() * 0.58f, bounds.getHeight() * 0.58f);
    const auto switchBounds = juce::Rectangle<float>(switchSize, switchSize).withCentre({ bounds.getCentreX(), bounds.getY() + switchSize * 0.63f });

    graphics.setColour(juce::Colour::fromRGB(12, 14, 16).withAlpha(0.18f));
    graphics.fillEllipse(switchBounds.translated(0.0f, 4.0f * scale).expanded(4.0f * scale));
    graphics.setColour(juce::Colour::fromRGB(227, 232, 231));
    graphics.fillEllipse(switchBounds.expanded(3.0f * scale));
    graphics.setColour(lit ? accentOrange : juce::Colour::fromRGB(35, 39, 43));
    graphics.fillEllipse(switchBounds);
    graphics.setColour(juce::Colours::white.withAlpha(lit ? 0.22f : highlighted || down ? 0.11f : 0.05f));
    graphics.fillEllipse(switchBounds.reduced(switchSize * 0.18f).translated(-switchSize * 0.08f, -switchSize * 0.10f));

    graphics.setColour(panelText);
    graphics.setFont(juce::FontOptions(12.0f * scale).withStyle("Bold"));
    graphics.drawText(button.getButtonText().toUpperCase(), bounds.withTrimmedTop(switchSize + 12.0f * scale).toNearestInt(), juce::Justification::centred);
}

void WeatheredLookAndFeel::drawButtonBackground(juce::Graphics& graphics, juce::Button& button, const juce::Colour& backgroundColour,
                                                bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    const auto scale = juce::jlimit(0.72f, 1.0f, std::min(bounds.getWidth() / 38.0f, bounds.getHeight() / 30.0f));
    const auto corner = 7.0f * scale;
    const auto isOn = button.getToggleState();

    graphics.setColour(juce::Colour::fromRGB(7, 9, 10).withAlpha(0.15f));
    graphics.fillRoundedRectangle(bounds.translated(0.0f, 2.0f * scale), corner);
    graphics.setColour(isOn ? button.findColour(juce::TextButton::buttonOnColourId) : backgroundColour);
    graphics.fillRoundedRectangle(bounds, corner);

    const auto highlight = isOn ? 0.16f : shouldDrawButtonAsHighlighted ? 0.10f : 0.035f;
    graphics.setColour(juce::Colours::white.withAlpha(highlight));
    graphics.fillRoundedRectangle(bounds.withTrimmedBottom(bounds.getHeight() * 0.55f), corner);
    graphics.setColour((shouldDrawButtonAsDown || isOn) ? accentOrange : juce::Colour::fromRGB(72, 81, 88));
    graphics.drawRoundedRectangle(bounds, corner, 1.2f * scale);
}

void WeatheredLookAndFeel::drawButtonText(juce::Graphics& graphics, juce::TextButton& button, bool, bool)
{
    const auto bounds = button.getLocalBounds().reduced(1);
    const auto text = button.getButtonText().toUpperCase();
    const auto fontSize = text.length() > 2 ? 12.5f : 17.0f;

    graphics.setColour(button.findColour(button.getToggleState() ? juce::TextButton::textColourOnId : juce::TextButton::textColourOffId));
    graphics.setFont(juce::FontOptions(juce::jmin(fontSize, static_cast<float>(bounds.getHeight()) * 0.58f)).withStyle("Bold"));
    graphics.drawFittedText(text, bounds, juce::Justification::centred, 1, 0.84f);
}

void WeatheredLookAndFeel::drawComboBox(juce::Graphics& graphics, int width, int height, bool isButtonDown,
                                        int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box)
{
    const auto visualScale = juce::jlimit(0.72f, 1.0f, static_cast<float>(height) / 30.0f);
    const auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)).reduced(0.5f);

    graphics.setColour(juce::Colour::fromRGB(7, 9, 10).withAlpha(0.16f));
    graphics.fillRoundedRectangle(bounds.translated(0.0f, 2.0f * visualScale), 5.0f * visualScale);
    graphics.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    graphics.fillRoundedRectangle(bounds, 5.0f * visualScale);
    graphics.setColour(juce::Colours::white.withAlpha(0.07f));
    graphics.fillRoundedRectangle(bounds.withTrimmedBottom(bounds.getHeight() * 0.52f), 5.0f * visualScale);
    graphics.setColour(isButtonDown ? accentOrange : box.findColour(juce::ComboBox::outlineColourId));
    graphics.drawRoundedRectangle(bounds, 5.0f * visualScale, 1.0f * visualScale);

    const auto buttonBounds = juce::Rectangle<float>(static_cast<float>(buttonX), static_cast<float>(buttonY),
                                                    static_cast<float>(buttonW), static_cast<float>(buttonH));
    juce::Path arrow;
    arrow.addTriangle(buttonBounds.getCentreX() - 4.5f * visualScale, buttonBounds.getCentreY() - 2.0f * visualScale,
                      buttonBounds.getCentreX() + 4.5f * visualScale, buttonBounds.getCentreY() - 2.0f * visualScale,
                      buttonBounds.getCentreX(), buttonBounds.getCentreY() + 4.0f * visualScale);
    graphics.setColour(box.findColour(juce::ComboBox::arrowColourId));
    graphics.fillPath(arrow);
}

void WeatheredLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    const auto visualScale = juce::jlimit(0.72f, 1.0f, static_cast<float>(box.getHeight()) / 30.0f);
    label.setBounds(juce::roundToInt(10.0f * visualScale), 1, box.getWidth() - juce::roundToInt(28.0f * visualScale), box.getHeight() - 2);
    label.setFont(juce::FontOptions(13.5f * visualScale).withStyle("Bold"));
    label.setJustificationType(juce::Justification::centredLeft);
}
} // namespace compressor808bytes
