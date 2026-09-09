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
    const auto y = face.getY() + face.getHeight() * (0.35f + 0.20f * centred * centred);
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

float reductionToProportion(float reductionDb) noexcept
{
    return valueToProportion(juce::jlimit(0.0f, meterRangeDb, reductionDb), reductionMarks);
}

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
    constexpr int segments = 52;
    for (int segment = 0; segment <= segments; ++segment)
    {
        const auto t = start + (end - start) * static_cast<float>(segment) / static_cast<float>(segments);
        const auto point = scalePointForProportion(face, t);
        segment == 0 ? path.startNewSubPath(point) : path.lineTo(point);
    }

    graphics.setColour(colour);
    graphics.strokePath(path, juce::PathStrokeType(thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

juce::Colour levelColour(float normalized) noexcept
{
    if (normalized > 0.92f)
        return juce::Colour::fromRGB(242, 82, 55);
    if (normalized > 0.78f)
        return juce::Colour::fromRGB(237, 190, 72);
    return juce::Colour::fromRGB(71, 211, 157);
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
        const auto scale = juce::jlimit(0.62f, 1.0f, std::min(bounds.getWidth() / 430.0f, bounds.getHeight() / 160.0f));
        const auto bezel = bounds.reduced(2.0f);
        auto face = bezel.reduced(24.0f * scale, 18.0f * scale);
        auto header = face.removeFromTop(23.0f * scale);
        const auto readoutArea = face.removeFromBottom(22.0f * scale);

        graphics.setColour(juce::Colour::fromRGB(7, 9, 10).withAlpha(0.24f));
        graphics.fillRoundedRectangle(bounds.translated(0.0f, 5.0f * scale), 9.0f * scale);

        juce::ColourGradient bezelGradient(juce::Colour::fromRGB(32, 36, 39), bezel.getX(), bezel.getY(),
                                           juce::Colour::fromRGB(11, 13, 15), bezel.getRight(), bezel.getBottom(), false);
        graphics.setGradientFill(bezelGradient);
        graphics.fillRoundedRectangle(bezel, 9.0f * scale);
        graphics.setColour(juce::Colour::fromRGB(78, 88, 94));
        graphics.drawRoundedRectangle(bezel, 9.0f * scale, 1.1f * scale);

        graphics.setColour(juce::Colour::fromRGB(12, 16, 18));
        graphics.fillRoundedRectangle(face.expanded(0.0f, 4.0f * scale), 5.0f * scale);
        graphics.setColour(juce::Colour::fromRGB(36, 44, 48).withAlpha(0.45f));
        for (int line = 1; line < 6; ++line)
        {
            const auto x = face.getX() + face.getWidth() * static_cast<float>(line) / 6.0f;
            graphics.drawVerticalLine(juce::roundToInt(x), face.getY(), face.getBottom());
        }
        graphics.setColour(juce::Colour::fromRGB(36, 44, 48).withAlpha(0.25f));
        for (int line = 1; line < 3; ++line)
        {
            const auto y = face.getY() + face.getHeight() * static_cast<float>(line) / 3.0f;
            graphics.drawHorizontalLine(juce::roundToInt(y), face.getX(), face.getRight());
        }

        graphics.setFont(juce::FontOptions(11.0f * scale).withStyle("Bold"));
        graphics.setColour(juce::Colour::fromRGB(221, 230, 227));
        graphics.drawText(name.toUpperCase(), header.toNearestInt(), juce::Justification::centredLeft);
        graphics.setColour(juce::Colour::fromRGB(237, 190, 72));
        const auto modeText = isReductionMode ? "GAIN REDUCTION" : (meterMode == MeterMode::OutputPlus4 ? "+4 OUTPUT" : "+8 OUTPUT");
        graphics.drawText(modeText, header.toNearestInt(), juce::Justification::centredRight);

        const auto needleProportion = isReductionMode ? reductionToProportion(reductionDb) : outputToProportion(vuDb);
        const auto needleEnd = scalePointForProportion(face, needleProportion);
        const auto pivot = juce::Point<float>(face.getCentreX(), face.getBottom() - 5.0f * scale);

        strokeMeterArc(graphics, face, 0.06f, 0.99f, juce::Colour::fromRGB(105, 119, 123).withAlpha(0.42f), 1.4f * scale);
        if (! isReductionMode)
            strokeMeterArc(graphics, face, outputToProportion(0.0f), 0.99f, juce::Colour::fromRGB(242, 82, 55).withAlpha(0.86f), 2.0f * scale);

        graphics.setFont(juce::FontOptions(9.4f * scale).withStyle("Bold"));
        const auto drawMarks = [&graphics, face, scale, isReductionMode] (const auto& marks)
        {
            for (const auto mark : marks)
            {
                const auto tickStart = scalePointForProportion(face, mark.proportion);
                const auto tickDirection = tickDirectionForProportion(mark.proportion);
                const auto tickHeight = (mark.major ? 19.0f : 11.0f) * scale;
                const auto tickEnd = tickStart + tickDirection * tickHeight;
                const auto hot = ! isReductionMode && mark.valueDb > 0.0f;

                graphics.setColour(hot ? juce::Colour::fromRGB(242, 82, 55) : juce::Colour::fromRGB(215, 224, 221));
                graphics.drawLine(tickStart.x, tickStart.y, tickEnd.x, tickEnd.y, (mark.major ? 1.6f : 0.9f) * scale);
                if (mark.label[0] != '\0')
                {
                    const auto labelPoint = tickStart + tickDirection * (tickHeight + 7.0f * scale);
                    graphics.drawText(mark.label, juce::Rectangle<float>(labelPoint.x - 17.0f * scale, labelPoint.y - 7.5f * scale,
                                                                          34.0f * scale, 15.0f * scale),
                                      juce::Justification::centred);
                }
            }
        };

        if (isReductionMode)
            drawMarks(reductionMarks);
        else
            drawMarks(outputMarks);

        auto needleDirection = needleEnd - pivot;
        const auto needleLength = std::sqrt(needleDirection.x * needleDirection.x + needleDirection.y * needleDirection.y);
        needleDirection = needleLength > 0.0f ? needleDirection / needleLength : juce::Point<float>(0.0f, -1.0f);
        const auto needleBase = pivot - needleDirection * (9.0f * scale);
        graphics.setColour(juce::Colour::fromRGB(0, 0, 0).withAlpha(0.34f));
        graphics.drawLine(needleBase.x + 1.5f * scale, needleBase.y + 2.0f * scale, needleEnd.x + 1.5f * scale, needleEnd.y + 2.0f * scale, 3.2f * scale);
        graphics.setColour(juce::Colour::fromRGB(237, 190, 72));
        graphics.drawLine(needleBase.x, needleBase.y, needleEnd.x, needleEnd.y, 2.2f * scale);
        graphics.setColour(juce::Colour::fromRGB(242, 102, 53));
        graphics.fillEllipse(juce::Rectangle<float>(11.0f * scale, 11.0f * scale).withCentre(pivot));

        const auto numericValue = isReductionMode ? reductionDb : vuDb;
        const auto valueText = (isReductionMode ? juce::String(numericValue, 1) + " dB GR" : juce::String(numericValue, 1) + " VU");
        graphics.setColour(juce::Colour::fromRGB(71, 211, 157));
        graphics.setFont(juce::FontOptions(12.0f * scale).withStyle("Bold"));
        graphics.drawText(valueText, readoutArea.toNearestInt(), juce::Justification::centred);
        return;
    }

    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    const auto value = source->load(std::memory_order_relaxed);
    const auto normalized = juce::jlimit(0.0f, 1.0f, (value + 60.0f) / 60.0f);
    const auto scale = juce::jlimit(0.62f, 1.0f, std::min(bounds.getWidth() / 54.0f, bounds.getHeight() / 160.0f));
    auto meter = bounds;
    auto labelArea = meter.removeFromTop(15.0f * scale);
    auto readoutArea = meter.removeFromBottom(17.0f * scale);
    auto well = meter.reduced(7.0f * scale, 4.0f * scale);

    graphics.setColour(juce::Colour::fromRGB(226, 231, 230));
    graphics.fillRoundedRectangle(bounds, 5.0f * scale);
    graphics.setColour(juce::Colour::fromRGB(13, 16, 18));
    graphics.fillRoundedRectangle(well, 3.0f * scale);
    graphics.setColour(juce::Colour::fromRGB(78, 88, 94));
    graphics.drawRoundedRectangle(well, 3.0f * scale, 1.0f * scale);

    auto fill = well.reduced(3.0f * scale);
    fill = fill.removeFromBottom(fill.getHeight() * normalized);
    graphics.setColour(levelColour(normalized));
    graphics.fillRoundedRectangle(fill, 2.0f * scale);

    graphics.setColour(juce::Colours::white.withAlpha(0.08f));
    graphics.fillRect(well.withTrimmedRight(well.getWidth() * 0.52f));
    graphics.setColour(juce::Colour::fromRGB(34, 39, 43));
    graphics.setFont(juce::FontOptions(9.2f * scale).withStyle("Bold"));
    graphics.drawText(name, labelArea.toNearestInt(), juce::Justification::centred);
    const auto readout = value <= -99.0f ? juce::String("-inf") : juce::String(juce::roundToInt(value));
    graphics.drawText(readout, readoutArea.toNearestInt(), juce::Justification::centred);
}
} // namespace compressor808bytes
