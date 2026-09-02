#include "MeterComponent.h"

#include <array>
#include <cmath>

namespace compressor808bytes
{
namespace
{
constexpr float meterRangeDb = 20.0f;

struct MeterMark
{
    float valueDb;
};

constexpr std::array<MeterMark, 9> reductionMarks {{
    { 0.0f },
    { 2.5f },
    { 5.0f },
    { 7.5f },
    { 10.0f },
    { 12.5f },
    { 15.0f },
    { 17.5f },
    { 20.0f }
}};

constexpr std::array<MeterMark, 5> labelledReductionMarks {{
    { 0.0f },
    { 5.0f },
    { 10.0f },
    { 15.0f },
    { 20.0f },
}};

float reductionToX(float reductionDb, juce::Rectangle<float> rail) noexcept
{
    const auto value = juce::jlimit(0.0f, meterRangeDb, reductionDb);
    return rail.getRight() - (value / meterRangeDb) * rail.getWidth();
}
} // namespace

MeterComponent::MeterComponent(const juce::String& meterName, const std::atomic<float>& meterSource, bool reduction)
    : name(meterName), source(meterSource), isGainReduction(reduction) {}

void MeterComponent::paint(juce::Graphics& graphics)
{
    if (isGainReduction)
    {
        const auto bounds = getLocalBounds().toFloat().reduced(4.0f);
        const auto reductionDb = juce::jlimit(0.0f, meterRangeDb, source.load(std::memory_order_relaxed));
        const auto bezel = bounds.reduced(2.0f);
        const auto scale = juce::jlimit(0.62f, 1.0f, std::min(bounds.getWidth() / 330.0f, bounds.getHeight() / 160.0f));
        const auto face = bezel.reduced(30.0f * scale, 28.0f * scale).withTrimmedBottom(8.0f * scale);
        graphics.setColour(juce::Colour::fromRGB(5, 5, 5).withAlpha(0.45f));
        graphics.fillRoundedRectangle(bounds.translated(3.0f, 5.0f), 8.0f);
        graphics.setColour(juce::Colour::fromRGB(18, 18, 16));
        graphics.fillRoundedRectangle(bezel, 8.0f);
        graphics.setColour(juce::Colour::fromRGB(70, 62, 50));
        graphics.drawRoundedRectangle(bezel, 8.0f, 1.4f);

        for (auto point : { bezel.getTopLeft() + juce::Point<float>(17.0f * scale, 17.0f * scale),
                            bezel.getTopRight() + juce::Point<float>(-17.0f * scale, 17.0f * scale),
                            bezel.getBottomLeft() + juce::Point<float>(17.0f * scale, -17.0f * scale),
                            bezel.getBottomRight() + juce::Point<float>(-17.0f * scale, -17.0f * scale) })
        {
            graphics.setColour(juce::Colour::fromRGB(7, 7, 6));
            graphics.fillEllipse(juce::Rectangle<float>(9.0f * scale, 9.0f * scale).withCentre(point));
            graphics.setColour(juce::Colour::fromRGB(74, 62, 43));
            graphics.drawEllipse(juce::Rectangle<float>(9.0f * scale, 9.0f * scale).withCentre(point), 1.0f);
        }

        graphics.setColour(juce::Colour::fromRGB(202, 157, 78));
        graphics.fillRoundedRectangle(face, 5.0f);
        graphics.setColour(juce::Colour::fromRGB(71, 48, 26).withAlpha(0.16f));
        for (int mark = 0; mark < 90; ++mark)
        {
            const auto x = face.getX() + static_cast<float>((mark * 29) % juce::jmax(1, static_cast<int>(face.getWidth())));
            const auto y = face.getY() + static_cast<float>((mark * 37) % juce::jmax(1, static_cast<int>(face.getHeight())));
            graphics.fillEllipse(x, y, 1.0f + static_cast<float>(mark % 3), 0.8f + static_cast<float>(mark % 2));
        }
        graphics.setColour(juce::Colours::white.withAlpha(0.13f));
        graphics.fillRoundedRectangle(face.withTrimmedBottom(face.getHeight() * 0.57f), 5.0f);
        graphics.setColour(juce::Colour::fromRGB(64, 42, 25));
        graphics.drawRoundedRectangle(face, 5.0f, 1.5f);

        const auto rail = face.reduced(42.0f * scale, 0.0f);
        const auto trackY = face.getY() + face.getHeight() * 0.42f;
        const auto needleX = reductionToX(reductionDb, rail);

        graphics.setColour(juce::Colour::fromRGB(80, 55, 29).withAlpha(0.38f));
        graphics.fillRect(juce::Rectangle<float>(rail.getX(), trackY - 2.0f, rail.getWidth(), 4.0f));
        graphics.setColour(juce::Colour::fromRGB(50, 31, 20).withAlpha(0.44f));
        graphics.fillRect(juce::Rectangle<float>(needleX, trackY - 4.0f, rail.getRight() - needleX, 8.0f));

        for (const auto mark : reductionMarks)
        {
            const auto tickX = reductionToX(mark.valueDb, rail);
            const auto tickHeight = (mark.valueDb == 0.0f || std::abs(mark.valueDb) == 20.0f ? 30.0f : 20.0f) * scale;
            graphics.setColour(juce::Colour::fromRGB(49, 35, 23));
            graphics.drawLine(tickX, trackY - tickHeight * 0.5f, tickX, trackY + tickHeight * 0.5f, mark.valueDb == 0.0f ? 2.2f : 1.2f);
        }

        graphics.setFont(juce::FontOptions(12.0f * scale).withStyle("Bold"));
        for (const auto mark : labelledReductionMarks)
        {
            const auto markX = reductionToX(mark.valueDb, rail);
            const auto tickHeight = (mark.valueDb == 0.0f ? 34.0f : 28.0f) * scale;
            graphics.setColour(juce::Colour::fromRGB(35, 25, 16));
            graphics.drawLine(markX, trackY - tickHeight * 0.5f, markX, trackY + tickHeight * 0.5f, 2.2f);
        }

        for (const auto mark : labelledReductionMarks)
        {
            const auto markX = reductionToX(mark.valueDb, rail);
            graphics.setColour(juce::Colour::fromRGB(35, 25, 16));
            const auto label = juce::String(juce::roundToInt(mark.valueDb));
            graphics.drawText(label, juce::Rectangle<float>(markX - 20.0f * scale, trackY - 34.0f * scale, 40.0f * scale, 16.0f * scale), juce::Justification::centred);
        }

        juce::Path pointer;
        pointer.addTriangle(needleX - 6.0f * scale, trackY - 22.0f * scale, needleX + 6.0f * scale, trackY - 22.0f * scale, needleX, trackY - 11.0f * scale);
        pointer.addTriangle(needleX - 6.0f * scale, trackY + 22.0f * scale, needleX + 6.0f * scale, trackY + 22.0f * scale, needleX, trackY + 11.0f * scale);
        graphics.setColour(juce::Colour::fromRGB(40, 20, 14).withAlpha(0.35f));
        graphics.strokePath(pointer, juce::PathStrokeType(3.0f * scale));
        graphics.setColour(juce::Colour::fromRGB(66, 35, 20));
        graphics.drawLine(needleX, trackY - 24.0f * scale, needleX, trackY + 24.0f * scale, 2.4f * scale);
        graphics.fillPath(pointer);

        graphics.setColour(juce::Colour::fromRGB(49, 35, 23));
        graphics.setFont(juce::FontOptions(12.0f * scale).withStyle("Bold"));
        graphics.drawText("GAIN REDUCTION", juce::Rectangle<float>(rail.getX(), trackY + 25.0f * scale, rail.getWidth(), 18.0f * scale).toNearestInt(), juce::Justification::centred);
        graphics.setFont(juce::FontOptions(14.0f * scale).withStyle("Bold"));
        graphics.drawText(juce::String(reductionDb, 1) + " dB", juce::Rectangle<float>(rail.getX(), face.getBottom() - 22.0f * scale, rail.getWidth(), 20.0f * scale).toNearestInt(), juce::Justification::centred);
        return;
    }

    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    const auto value = source.load(std::memory_order_relaxed);
    const auto normalized = juce::jlimit(0.0f, 1.0f, (value + 60.0f) / 60.0f);
    auto meter = bounds;
    auto labelArea = meter.removeFromTop(15.0f);
    auto readoutArea = meter.removeFromBottom(16.0f);
    auto well = meter.reduced(6.0f, 3.0f);

    graphics.setColour(juce::Colour::fromRGB(129, 116, 88).withAlpha(0.85f));
    graphics.fillRoundedRectangle(bounds, 3.0f);
    graphics.setColour(juce::Colour::fromRGB(35, 30, 23));
    graphics.fillRoundedRectangle(well, 2.0f);

    auto fill = well.reduced(3.0f);
    fill = fill.removeFromBottom(fill.getHeight() * normalized);
    graphics.setColour(normalized > 0.84f ? juce::Colour::fromRGB(179, 54, 26) : juce::Colour::fromRGB(224, 165, 76));
    graphics.fillRoundedRectangle(fill, 1.5f);

    graphics.setColour(juce::Colours::white.withAlpha(0.14f));
    graphics.fillRect(well.withTrimmedRight(well.getWidth() * 0.55f));
    graphics.setColour(juce::Colour::fromRGB(37, 31, 24));
    graphics.setFont(juce::FontOptions(9.0f).withStyle("Bold"));
    graphics.drawText(name, labelArea.toNearestInt(), juce::Justification::centred);
    const auto readout = value <= -99.0f ? juce::String("-inf") : juce::String(juce::roundToInt(value));
    graphics.drawText(readout, readoutArea.toNearestInt(), juce::Justification::centred);
}
} // namespace compressor808bytes
