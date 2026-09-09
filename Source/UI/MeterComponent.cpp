#include "MeterComponent.h"

#include <array>
#include <cmath>

namespace compressor808bytes
{
namespace
{
constexpr float meterRangeDb = 20.0f;
constexpr float vuMinimumDb = -20.0f;
constexpr float vuMaximumDb = 3.0f;

struct MeterMark
{
    float valueDb;
    float proportion;
    const char* label;
    bool major;
};

constexpr std::array<MeterMark, 9> reductionMarks {{
    { 0.0f, 0.94f, "0", true },
    { 1.0f, 0.84f, "1", true },
    { 2.0f, 0.76f, "", false },
    { 3.0f, 0.68f, "3", true },
    { 5.0f, 0.56f, "5", true },
    { 7.0f, 0.46f, "7", true },
    { 10.0f, 0.33f, "10", true },
    { 15.0f, 0.18f, "", false },
    { 20.0f, 0.06f, "20", true }
}};

constexpr std::array<MeterMark, 11> outputMarks {{
    { -20.0f, 0.06f, "-20", true },
    { -10.0f, 0.28f, "-10", true },
    { -7.0f, 0.40f, "-7", true },
    { -5.0f, 0.51f, "-5", true },
    { -3.0f, 0.62f, "-3", true },
    { -2.0f, 0.70f, "", false },
    { -1.0f, 0.78f, "", false },
    { 0.0f, 0.85f, "0", true },
    { 1.0f, 0.91f, "", false },
    { 2.0f, 0.96f, "", false },
    { 3.0f, 0.99f, "+3", true }
}};

juce::Point<float> scalePointForProportion(juce::Rectangle<float> face, float proportion) noexcept
{
    const auto t = juce::jlimit(0.0f, 1.0f, proportion);
    const auto centred = (t - 0.5f) * 2.0f;
    const auto x = face.getX() + face.getWidth() * (0.08f + 0.84f * t);
    const auto y = face.getY() + face.getHeight() * (0.32f + 0.18f * centred * centred);
    return { x, y };
}

juce::Point<float> tickDirectionForProportion(float proportion) noexcept
{
    const auto t = juce::jlimit(0.0f, 1.0f, proportion);
    auto normal = juce::Point<float>((0.5f - t) * 0.34f, 1.0f);
    const auto length = std::sqrt(normal.x * normal.x + normal.y * normal.y);
    return length > 0.0f ? normal / length : juce::Point<float>(0.0f, 1.0f);
}

template <size_t size>
float valueToProportion(float valueDb, const std::array<MeterMark, size>& marks) noexcept
{
    if (valueDb <= marks.front().valueDb)
        return marks.front().proportion;

    if (valueDb >= marks.back().valueDb)
        return marks.back().proportion;

    for (size_t index = 1; index < marks.size(); ++index)
    {
        const auto& lower = marks[index - 1];
        const auto& upper = marks[index];

        if (valueDb <= upper.valueDb)
        {
            const auto amount = (valueDb - lower.valueDb) / (upper.valueDb - lower.valueDb);
            return lower.proportion + (upper.proportion - lower.proportion) * amount;
        }
    }

    return marks.back().proportion;
}

float reductionToProportion(float reductionDb) noexcept { return valueToProportion(juce::jlimit(0.0f, meterRangeDb, reductionDb), reductionMarks); }

float outputToProportion(float vuDb) noexcept
{
    return valueToProportion(juce::jlimit(vuMinimumDb, vuMaximumDb, vuDb), outputMarks);
}

float outputLevelToVu(float outputLevelDb, MeterMode mode) noexcept
{
    const auto calibrationDb = mode == MeterMode::OutputPlus8 ? 14.0f : 18.0f;
    return outputLevelDb + calibrationDb;
}

void strokeMeterArc(juce::Graphics& graphics, juce::Rectangle<float> face, float start, float end,
                    juce::Colour colour, float thickness) noexcept
{
    juce::Path path;
    constexpr int segments = 44;
    for (int segment = 0; segment <= segments; ++segment)
    {
        const auto t = start + (end - start) * static_cast<float>(segment) / static_cast<float>(segments);
        const auto point = scalePointForProportion(face, t);
        segment == 0 ? path.startNewSubPath(point) : path.lineTo(point);
    }

    graphics.setColour(colour);
    graphics.strokePath(path, juce::PathStrokeType(thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
} // namespace

MeterComponent::MeterComponent(const juce::String& meterName, const std::atomic<float>& meterSource, bool reduction)
    : name(meterName),
      source(&meterSource),
      outputSource(&meterSource),
      gainReductionSource(&meterSource),
      isLargeMeter(reduction)
{
}

MeterComponent::MeterComponent(const juce::String& meterName, const std::atomic<float>& inputMeterSource,
                               const std::atomic<float>& outputMeterSource, const std::atomic<float>& reductionMeterSource)
    : name(meterName),
      outputSource(&outputMeterSource),
      gainReductionSource(&reductionMeterSource),
      isLargeMeter(true)
{
    juce::ignoreUnused(inputMeterSource);
}

void MeterComponent::setMeterMode(MeterMode newMode) noexcept
{
    meterMode = newMode;
    repaint();
}

void MeterComponent::paint(juce::Graphics& graphics)
{
    if (isLargeMeter)
    {
        const auto bounds = getLocalBounds().toFloat().reduced(4.0f);
        const auto reductionDb = juce::jlimit(0.0f, meterRangeDb, gainReductionSource->load(std::memory_order_relaxed));
        const auto vuDb = outputLevelToVu(outputSource->load(std::memory_order_relaxed), meterMode);
        const auto isReductionMode = meterMode == MeterMode::GainReduction;
        const auto bezel = bounds.reduced(2.0f);
        const auto scale = juce::jlimit(0.62f, 1.0f, std::min(bounds.getWidth() / 330.0f, bounds.getHeight() / 160.0f));
        const auto face = bezel.reduced(31.0f * scale, 20.0f * scale).withTrimmedBottom(4.0f * scale);
        graphics.setColour(juce::Colour::fromRGB(5, 5, 5).withAlpha(0.45f));
        graphics.fillRoundedRectangle(bounds.translated(3.0f * scale, 5.0f * scale), 8.0f * scale);
        graphics.setColour(juce::Colour::fromRGB(18, 18, 16));
        graphics.fillRoundedRectangle(bezel, 8.0f * scale);
        graphics.setColour(juce::Colour::fromRGB(70, 62, 50));
        graphics.drawRoundedRectangle(bezel, 8.0f * scale, 1.4f * scale);

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
        graphics.fillRoundedRectangle(face, 5.0f * scale);
        graphics.setColour(juce::Colour::fromRGB(71, 48, 26).withAlpha(0.16f));
        for (int mark = 0; mark < 90; ++mark)
        {
            const auto x = face.getX() + static_cast<float>((mark * 29) % juce::jmax(1, static_cast<int>(face.getWidth())));
            const auto y = face.getY() + static_cast<float>((mark * 37) % juce::jmax(1, static_cast<int>(face.getHeight())));
            graphics.fillEllipse(x, y, (1.0f + static_cast<float>(mark % 3)) * scale, (0.8f + static_cast<float>(mark % 2)) * scale);
        }
        graphics.setColour(juce::Colours::white.withAlpha(0.13f));
        graphics.fillRoundedRectangle(face.withTrimmedBottom(face.getHeight() * 0.57f), 5.0f * scale);
        graphics.setColour(juce::Colour::fromRGB(64, 42, 25));
        graphics.drawRoundedRectangle(face, 5.0f * scale, 1.5f * scale);

        const auto needleProportion = isReductionMode ? reductionToProportion(reductionDb) : outputToProportion(vuDb);
        const auto needleEnd = scalePointForProportion(face, needleProportion);
        const auto pivot = juce::Point<float>(face.getCentreX(), face.getBottom() - 11.0f * scale);

        strokeMeterArc(graphics, face, 0.06f, 0.99f, juce::Colour::fromRGB(80, 55, 29).withAlpha(0.34f), 1.4f * scale);
        if (! isReductionMode)
            strokeMeterArc(graphics, face, outputToProportion(0.0f), 0.99f, juce::Colour::fromRGB(154, 31, 20).withAlpha(0.76f), 2.0f * scale);

        graphics.setFont(juce::FontOptions(9.8f * scale).withStyle("Bold"));

        if (isReductionMode)
        {
            for (const auto mark : reductionMarks)
            {
                const auto markProportion = mark.proportion;
                const auto tickHeight = (mark.major ? 22.0f : 14.0f) * scale;
                const auto tickStart = scalePointForProportion(face, markProportion);
                const auto tickDirection = tickDirectionForProportion(markProportion);
                const auto tickEnd = tickStart + tickDirection * tickHeight;
                graphics.setColour(juce::Colour::fromRGB(35, 25, 16));
                graphics.drawLine(tickStart.x, tickStart.y, tickEnd.x, tickEnd.y, (mark.major ? 2.0f : 1.1f) * scale);
                if (mark.label[0] != '\0')
                {
                    const auto labelPoint = tickStart + tickDirection * (tickHeight + 7.0f * scale);
                    graphics.drawText(mark.label, juce::Rectangle<float>(labelPoint.x - 17.0f * scale, labelPoint.y - 7.5f * scale, 34.0f * scale, 15.0f * scale), juce::Justification::centred);
                }
            }
        }
        else
        {
            for (const auto mark : outputMarks)
            {
                const auto markProportion = mark.proportion;
                const auto tickHeight = (mark.major ? 22.0f : 14.0f) * scale;
                const auto tickStart = scalePointForProportion(face, markProportion);
                const auto tickDirection = tickDirectionForProportion(markProportion);
                const auto tickEnd = tickStart + tickDirection * tickHeight;
                graphics.setColour(mark.valueDb > 0.0f ? juce::Colour::fromRGB(154, 31, 20) : juce::Colour::fromRGB(35, 25, 16));
                graphics.drawLine(tickStart.x, tickStart.y, tickEnd.x, tickEnd.y, (mark.major ? 2.0f : 1.1f) * scale);
                if (mark.label[0] != '\0')
                {
                    const auto labelPoint = tickStart + tickDirection * (tickHeight + 7.0f * scale);
                    graphics.drawText(mark.label, juce::Rectangle<float>(labelPoint.x - 17.0f * scale, labelPoint.y - 7.5f * scale, 34.0f * scale, 15.0f * scale), juce::Justification::centred);
                }
            }
        }

        auto needleDirection = needleEnd - pivot;
        const auto needleLength = std::sqrt(needleDirection.x * needleDirection.x + needleDirection.y * needleDirection.y);
        needleDirection = needleLength > 0.0f ? needleDirection / needleLength : juce::Point<float>(0.0f, -1.0f);
        const auto needleBase = pivot - needleDirection * (10.0f * scale);
        graphics.setColour(juce::Colour::fromRGB(40, 20, 14).withAlpha(0.35f));
        graphics.drawLine(needleBase.x + 1.4f * scale, needleBase.y + 2.0f * scale, needleEnd.x + 1.4f * scale, needleEnd.y + 2.0f * scale, 2.8f * scale);
        graphics.setColour(juce::Colour::fromRGB(66, 35, 20));
        graphics.drawLine(needleBase.x, needleBase.y, needleEnd.x, needleEnd.y, 1.8f * scale);
        graphics.setColour(juce::Colour::fromRGB(32, 18, 12));
        graphics.fillEllipse(juce::Rectangle<float>(9.0f * scale, 9.0f * scale).withCentre(pivot));

        return;
    }

    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    const auto value = source->load(std::memory_order_relaxed);
    const auto normalized = juce::jlimit(0.0f, 1.0f, (value + 60.0f) / 60.0f);
    const auto scale = juce::jlimit(0.62f, 1.0f, std::min(bounds.getWidth() / 54.0f, bounds.getHeight() / 160.0f));
    auto meter = bounds;
    auto labelArea = meter.removeFromTop(15.0f * scale);
    auto readoutArea = meter.removeFromBottom(16.0f * scale);
    auto well = meter.reduced(6.0f * scale, 3.0f * scale);

    graphics.setColour(juce::Colour::fromRGB(129, 116, 88).withAlpha(0.85f));
    graphics.fillRoundedRectangle(bounds, 3.0f * scale);
    graphics.setColour(juce::Colour::fromRGB(35, 30, 23));
    graphics.fillRoundedRectangle(well, 2.0f * scale);

    auto fill = well.reduced(3.0f * scale);
    fill = fill.removeFromBottom(fill.getHeight() * normalized);
    graphics.setColour(normalized > 0.84f ? juce::Colour::fromRGB(179, 54, 26) : juce::Colour::fromRGB(224, 165, 76));
    graphics.fillRoundedRectangle(fill, 1.5f * scale);

    graphics.setColour(juce::Colours::white.withAlpha(0.14f));
    graphics.fillRect(well.withTrimmedRight(well.getWidth() * 0.55f));
    graphics.setColour(juce::Colour::fromRGB(37, 31, 24));
    graphics.setFont(juce::FontOptions(9.0f * scale).withStyle("Bold"));
    graphics.drawText(name, labelArea.toNearestInt(), juce::Justification::centred);
    const auto readout = value <= -99.0f ? juce::String("-inf") : juce::String(juce::roundToInt(value));
    graphics.drawText(readout, readoutArea.toNearestInt(), juce::Justification::centred);
}
} // namespace compressor808bytes
