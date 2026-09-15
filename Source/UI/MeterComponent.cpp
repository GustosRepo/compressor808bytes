#include "MeterComponent.h"

#include <algorithm>
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
    { 7.0f, 0.45f, "7", true },
    { 10.0f, 0.32f, "10", true },
    { 15.0f, 0.18f, "", false },
    { 20.0f, 0.06f, "20", true }
}};

constexpr std::array<MeterMark, 11> outputMarks {{
    { -20.0f, 0.06f, "-20", true },
    { -10.0f, 0.27f, "-10", true },
    { -7.0f, 0.39f, "-7", true },
    { -5.0f, 0.50f, "-5", true },
    { -3.0f, 0.61f, "-3", true },
    { -2.0f, 0.69f, "", false },
    { -1.0f, 0.77f, "", false },
    { 0.0f, 0.85f, "0", true },
    { 1.0f, 0.91f, "", false },
    { 2.0f, 0.96f, "", false },
    { 3.0f, 0.99f, "+3", true }
}};

juce::Point<float> scalePointForProportion(juce::Rectangle<float> face, float proportion) noexcept
{
    const auto t = juce::jlimit(0.0f, 1.0f, proportion);
    const auto centred = (t - 0.5f) * 2.0f;
    return { face.getX() + face.getWidth() * (0.07f + 0.86f * t),
             face.getY() + face.getHeight() * (0.33f + 0.16f * centred * centred) };
}

juce::Point<float> meterPivotForFace(juce::Rectangle<float> face) noexcept
{
    return { face.getCentreX(), face.getBottom() + face.getHeight() * 0.25f };
}

juce::Point<float> tickDirectionForProportion(juce::Rectangle<float> face, float proportion) noexcept
{
    const auto t = juce::jlimit(0.0f, 1.0f, proportion);
    const auto neighbour = scalePointForProportion(face, juce::jlimit(0.0f, 1.0f, t + 0.01f));
    const auto point = scalePointForProportion(face, t);
    const auto tangent = neighbour - point;
    auto normal = juce::Point<float>(-tangent.y, tangent.x);
    if (normal.y < 0.0f)
        normal *= -1.0f;
    const auto length = std::sqrt(normal.x * normal.x + normal.y * normal.y);
    return length > 0.0f ? normal / length : juce::Point<float>(0.0f, 1.0f);
}

juce::Point<float> arcPointForProportion(juce::Rectangle<float> face, float proportion, float radiusRatio) noexcept
{
    const auto pivot = meterPivotForFace(face);
    return pivot + (scalePointForProportion(face, proportion) - pivot) * radiusRatio;
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

void drawMeterArc(juce::Graphics& graphics, juce::Rectangle<float> face, float startProportion, float endProportion,
                  juce::Colour colour, float radiusRatio, float thickness) noexcept
{
    juce::Path path;
    constexpr int segments = 72;

    for (int segment = 0; segment <= segments; ++segment)
    {
        const auto amount = static_cast<float>(segment) / static_cast<float>(segments);
        const auto point = arcPointForProportion(face, startProportion + (endProportion - startProportion) * amount, radiusRatio);
        segment == 0 ? path.startNewSubPath(point) : path.lineTo(point);
    }

    graphics.setColour(colour);
    graphics.strokePath(path, juce::PathStrokeType(thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

template <size_t size>
void drawMeterTicks(juce::Graphics& graphics, juce::Rectangle<float> face, const std::array<MeterMark, size>& marks,
                    bool isReductionMode, float scale) noexcept
{
    for (const auto& mark : marks)
    {
        const auto outer = arcPointForProportion(face, mark.proportion, 1.0f);
        const auto direction = tickDirectionForProportion(face, mark.proportion);
        const auto inner = outer + direction * ((mark.major ? 17.0f : 10.0f) * scale);
        const auto tickColour = ! isReductionMode && mark.valueDb > 0.0f ? juce::Colour::fromRGB(174, 38, 28)
                                                                          : juce::Colour::fromRGB(25, 26, 24);
        graphics.setColour(tickColour);
        graphics.drawLine(inner.x, inner.y, outer.x, outer.y, (mark.major ? 1.5f : 0.9f) * scale);

        if (mark.label[0] != '\0')
        {
            const auto labelPos = outer + direction * ((mark.major ? 28.0f : 20.0f) * scale);
            graphics.setFont(juce::FontOptions(mark.major ? 11.4f * scale : 8.6f * scale).withStyle("Bold"));
            graphics.setColour(! isReductionMode && mark.valueDb > 0.0f ? juce::Colour::fromRGB(174, 38, 28)
                                                                          : juce::Colour::fromRGB(25, 26, 24));
            graphics.drawText(mark.label,
                              juce::Rectangle<float>(labelPos.x - 18.0f * scale,
                                                     labelPos.y - 9.0f * scale,
                                                     36.0f * scale,
                                                     18.0f * scale),
                              juce::Justification::centred);
        }
    }
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
        const auto bounds = getLocalBounds().toFloat().reduced(5.0f);
        const auto reductionDb = juce::jlimit(0.0f, meterRangeDb, gainReductionSource->load(std::memory_order_relaxed));
        const auto vuDb = outputLevelToVu(outputSource->load(std::memory_order_relaxed), meterMode);
        const auto isReductionMode = meterMode == MeterMode::GainReduction;
        const auto scale = juce::jlimit(0.7f, 1.0f, std::min(bounds.getWidth() / 420.0f, bounds.getHeight() / 150.0f));
        const auto bezel = bounds.reduced(2.0f);
        const auto facePlate = bezel.reduced(14.0f * scale, 12.0f * scale);
        const auto glass = facePlate.reduced(5.0f * scale);
        auto face = glass.reduced(18.0f * scale, 11.0f * scale);
        auto header = face.removeFromTop(21.0f * scale);
        auto readoutArea = face.removeFromBottom(23.0f * scale);
        const auto scaleFace = face.reduced(3.0f * scale, 0.0f);

        graphics.setColour(juce::Colour::fromRGB(7, 9, 10).withAlpha(0.26f));
        graphics.fillRoundedRectangle(bounds.translated(0.0f, 4.0f * scale), 11.0f * scale);

        juce::ColourGradient bezelGradient(juce::Colour::fromRGB(28, 32, 36), bezel.getX(), bezel.getY(),
                                           juce::Colour::fromRGB(10, 12, 14), bezel.getRight(), bezel.getBottom(), false);
        graphics.setGradientFill(bezelGradient);
        graphics.fillRoundedRectangle(bezel, 10.0f * scale);
        graphics.setColour(juce::Colour::fromRGB(96, 105, 109));
        graphics.drawRoundedRectangle(bezel, 10.0f * scale, 1.1f * scale);

        graphics.setColour(juce::Colour::fromRGB(12, 14, 15));
        graphics.fillRoundedRectangle(facePlate, 4.0f * scale);
        graphics.setColour(juce::Colour::fromRGB(79, 88, 91));
        graphics.drawRoundedRectangle(facePlate, 4.0f * scale, 1.0f * scale);

        juce::ColourGradient glassGradient(juce::Colour::fromRGB(248, 241, 215), glass.getX(), glass.getY(),
                                           juce::Colour::fromRGB(218, 204, 169), glass.getX(), glass.getBottom(), false);
        graphics.setGradientFill(glassGradient);
        graphics.fillRoundedRectangle(glass, 3.0f * scale);
        graphics.setColour(juce::Colour::fromRGB(92, 83, 66));
        graphics.drawRoundedRectangle(glass, 3.0f * scale, 1.0f * scale);

        graphics.setColour(juce::Colour::fromRGB(255, 255, 255).withAlpha(0.18f));
        graphics.fillRoundedRectangle(glass.withTrimmedBottom(glass.getHeight() * 0.62f).reduced(4.0f * scale), 2.0f * scale);

        graphics.setFont(juce::FontOptions(10.5f * scale).withStyle("Bold"));
        graphics.setColour(juce::Colour::fromRGB(31, 28, 24));
        graphics.drawText(isReductionMode ? "VU" : name.toUpperCase(), header.toNearestInt(), juce::Justification::centredLeft);
        graphics.setColour(juce::Colour::fromRGB(117, 35, 27));
        const auto modeText = isReductionMode ? "GAIN REDUCTION" : (meterMode == MeterMode::OutputPlus4 ? "+4 OUTPUT" : "+8 OUTPUT");
        graphics.drawText(modeText, header.toNearestInt(), juce::Justification::centredRight);

        drawMeterArc(graphics, scaleFace, 0.06f, 0.99f, juce::Colour::fromRGB(25, 26, 24), 1.0f, 1.2f * scale);
        drawMeterArc(graphics, scaleFace, 0.06f, 0.99f, juce::Colour::fromRGB(25, 26, 24).withAlpha(0.34f), 0.82f, 0.8f * scale);

        if (! isReductionMode)
        {
            drawMeterArc(graphics, scaleFace, outputToProportion(0.0f), outputToProportion(vuMaximumDb),
                         juce::Colour::fromRGB(174, 38, 28), 1.04f, 2.8f * scale);
        }

        if (isReductionMode)
            drawMeterTicks(graphics, scaleFace, reductionMarks, true, scale);
        else
            drawMeterTicks(graphics, scaleFace, outputMarks, false, scale);

        graphics.setColour(juce::Colour::fromRGB(35, 31, 27));
        graphics.setFont(juce::FontOptions(8.8f * scale).withStyle("Bold"));
        graphics.drawText(isReductionMode ? "DB" : "VU",
                          juce::Rectangle<float>(scaleFace.getCentreX() - 18.0f * scale,
                                                 scaleFace.getY() + scaleFace.getHeight() * 0.56f,
                                                 36.0f * scale,
                                                 14.0f * scale).toNearestInt(),
                          juce::Justification::centred);

        const auto valueForNeedle = isReductionMode ? reductionDb : vuDb;
        const auto needleProportion = isReductionMode ? reductionToProportion(valueForNeedle) : outputToProportion(valueForNeedle);
        const auto needle = arcPointForProportion(scaleFace, needleProportion, 0.84f);
        const auto pivot = meterPivotForFace(scaleFace);
        auto needleDirection = needle - pivot;
        const auto needleLength = std::sqrt(needleDirection.x * needleDirection.x + needleDirection.y * needleDirection.y);
        needleDirection = needleLength > 0.0f ? needleDirection / needleLength : juce::Point<float>(0.0f, -1.0f);
        const auto needleBase = pivot - needleDirection * (7.0f * scale);
        graphics.setColour(juce::Colour::fromRGB(0, 0, 0).withAlpha(0.20f));
        graphics.drawLine(needleBase.x + 1.0f * scale, needleBase.y + 1.5f * scale, needle.x + 1.0f * scale, needle.y + 1.5f * scale, 2.5f * scale);
        graphics.setColour(juce::Colour::fromRGB(22, 21, 19));
        graphics.drawLine(needleBase.x, needleBase.y, needle.x, needle.y, 1.8f * scale);
        graphics.setColour(juce::Colour::fromRGB(44, 39, 32));
        graphics.fillEllipse(juce::Rectangle<float>(17.0f * scale, 17.0f * scale).withCentre(pivot));
        graphics.setColour(juce::Colour::fromRGB(178, 163, 124));
        graphics.fillEllipse(juce::Rectangle<float>(7.0f * scale, 7.0f * scale).withCentre(pivot));

        const auto numericValue = isReductionMode ? reductionDb : vuDb;
        const auto valueText = isReductionMode ? juce::String(numericValue, 1) + " dB GR" : juce::String(numericValue, 1) + " VU";
        graphics.setColour(juce::Colour::fromRGB(31, 28, 24));
        graphics.setFont(juce::FontOptions(11.4f * scale).withStyle("Bold"));
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

    graphics.setColour(juce::Colour::fromRGB(71, 211, 157).withAlpha(0.12f));
    graphics.fillRect(well.withTrimmedRight(well.getWidth() * 0.52f));
    graphics.setColour(juce::Colour::fromRGB(34, 39, 43));
    graphics.setFont(juce::FontOptions(9.2f * scale).withStyle("Bold"));
    graphics.drawText(name, labelArea.toNearestInt(), juce::Justification::centred);
    const auto readout = value <= -99.0f ? juce::String("-inf") : juce::String(juce::roundToInt(value));
    graphics.drawText(readout, readoutArea.toNearestInt(), juce::Justification::centred);
}
} // namespace compressor808bytes
