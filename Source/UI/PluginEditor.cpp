#include "PluginEditor.h"

#include <array>
#include <cmath>

namespace compressor808bytes
{
namespace
{
struct RatioButtonSpec
{
    const char* text;
    float value;
};

constexpr std::array<RatioButtonSpec, 5> ratioButtonSpecs {{
    { "4", 4.0f },
    { "8", 8.0f },
    { "12", 12.0f },
    { "20", 20.0f },
    { "ALL", 21.0f }
}};

struct MeterModeButtonSpec
{
    const char* text;
    MeterMode mode;
};

constexpr std::array<MeterModeButtonSpec, 3> meterModeButtonSpecs {{
    { "GR", MeterMode::GainReduction },
    { "+4", MeterMode::OutputPlus4 },
    { "+8", MeterMode::OutputPlus8 }
}};

constexpr float designWidth = 1120.0f;
constexpr float designHeight = 760.0f;
constexpr float designAspectRatio = designWidth / designHeight;
constexpr int defaultEditorWidth = 820;
constexpr int defaultEditorHeight = static_cast<int>(static_cast<float>(defaultEditorWidth) / designAspectRatio + 0.5f);
constexpr int minimumEditorWidth = 820;
constexpr int minimumEditorHeight = static_cast<int>(static_cast<float>(minimumEditorWidth) / designAspectRatio + 0.5f);
constexpr int maximumEditorWidth = 1500;
constexpr int maximumEditorHeight = static_cast<int>(static_cast<float>(maximumEditorWidth) / designAspectRatio + 0.5f);

float editorScaleFor(int width, int height) noexcept
{
    if (width <= 0 || height <= 0)
        return 1.0f;

    return std::min(static_cast<float>(width) / designWidth, static_cast<float>(height) / designHeight);
}

juce::Rectangle<float> visualCanvasFor(juce::Rectangle<int> editorBounds) noexcept
{
    const auto scale = editorScaleFor(editorBounds.getWidth(), editorBounds.getHeight());
    return juce::Rectangle<float>(designWidth * scale, designHeight * scale).withCentre(editorBounds.toFloat().getCentre());
}

juce::Rectangle<int> designToEditorBounds(juce::Rectangle<float> canvas, juce::Rectangle<float> designBounds) noexcept
{
    const auto scale = canvas.getWidth() / designWidth;
    return juce::Rectangle<float>(canvas.getX() + designBounds.getX() * scale,
                                  canvas.getY() + designBounds.getY() * scale,
                                  designBounds.getWidth() * scale,
                                  designBounds.getHeight() * scale)
        .toNearestInt();
}

float nearestRatioButtonValue(float value) noexcept
{
    auto nearest = ratioButtonSpecs.front().value;
    auto nearestDistance = std::abs(value - nearest);

    for (const auto& spec : ratioButtonSpecs)
    {
        const auto distance = std::abs(value - spec.value);
        if (distance < nearestDistance)
        {
            nearest = spec.value;
            nearestDistance = distance;
        }
    }

    return nearest;
}

void drawScrew(juce::Graphics& graphics, juce::Point<float> centre, float radius)
{
    const auto scale = juce::jlimit(0.5f, 1.0f, radius / 11.0f);
    const auto bounds = juce::Rectangle<float>(radius * 2.0f, radius * 2.0f).withCentre(centre);
    graphics.setColour(juce::Colour::fromRGB(10, 12, 14).withAlpha(0.12f));
    graphics.fillEllipse(bounds.translated(0.0f, 1.5f * scale).expanded(radius * 0.28f));
    graphics.setColour(juce::Colour::fromRGB(214, 220, 219));
    graphics.fillEllipse(bounds.expanded(radius * 0.20f));
    graphics.setColour(juce::Colour::fromRGB(42, 49, 54));
    graphics.fillEllipse(bounds);
    graphics.setColour(juce::Colour::fromRGB(108, 119, 126));
    graphics.drawEllipse(bounds.reduced(1.0f * scale), 1.0f * scale);
    graphics.setColour(juce::Colour::fromRGB(11, 13, 15));
    graphics.drawLine(centre.x - radius * 0.36f, centre.y, centre.x + radius * 0.36f, centre.y, 1.4f * scale);
}

void drawAgeMarks(juce::Graphics& graphics, juce::Rectangle<float> area, float scale)
{
    graphics.setColour(juce::Colour::fromRGB(255, 255, 255).withAlpha(0.18f));
    graphics.drawHorizontalLine(static_cast<int>(area.getY() + 1.0f * scale), area.getX() + 12.0f * scale, area.getRight() - 12.0f * scale);
    graphics.setColour(juce::Colour::fromRGB(112, 124, 130).withAlpha(0.075f));
    for (int line = 1; line < 6; ++line)
    {
        const auto x = area.getX() + area.getWidth() * static_cast<float>(line) / 6.0f;
        graphics.drawVerticalLine(juce::roundToInt(x), area.getY() + 16.0f * scale, area.getBottom() - 16.0f * scale);
    }

    graphics.setColour(juce::Colour::fromRGB(112, 124, 130).withAlpha(0.055f));
    for (int line = 1; line < 4; ++line)
    {
        const auto y = area.getY() + area.getHeight() * static_cast<float>(line) / 4.0f;
        graphics.drawHorizontalLine(juce::roundToInt(y), area.getX() + 16.0f * scale, area.getRight() - 16.0f * scale);
    }
}

void drawGrimeAround(juce::Graphics& graphics, juce::Point<float> centre, float radius, float intensity)
{
    graphics.setColour(juce::Colour::fromRGB(255, 255, 255).withAlpha(0.20f * intensity));
    graphics.drawEllipse(juce::Rectangle<float>(radius * 2.0f, radius * 2.0f).withCentre(centre), 1.2f * intensity);
    graphics.setColour(juce::Colour::fromRGB(42, 49, 54).withAlpha(0.08f * intensity));
    graphics.fillEllipse(juce::Rectangle<float>(radius * 1.78f, radius * 1.78f).withCentre(centre));
}

void drawKnobGrime(juce::Graphics& graphics, const KnobComponent& knob, float multiplier)
{
    const auto knobBounds = knob.getBounds().toFloat();
    const auto sliderBounds = knob.slider.getBounds().toFloat().translated(knobBounds.getX(), knobBounds.getY());
    const auto side = std::min(sliderBounds.getWidth(), sliderBounds.getHeight());
    const auto scale = juce::jlimit(0.45f, 1.0f, side / 128.0f);
    const auto radius = side * multiplier * (0.76f + 0.24f * scale);
    drawGrimeAround(graphics, sliderBounds.getCentre(), radius, scale);
}

void drawBottomRailGrime(juce::Graphics& graphics, juce::Rectangle<float> rail, float scale)
{
    graphics.setColour(juce::Colour::fromRGB(63, 72, 78).withAlpha(0.42f));
    graphics.drawHorizontalLine(static_cast<int>(rail.getY() + 1.0f * scale), rail.getX() + 12.0f * scale, rail.getRight() - 12.0f * scale);

    for (int stripe = 0; stripe < 4; ++stripe)
    {
        const auto x = rail.getX() + 46.0f * scale + static_cast<float>(stripe) * 9.0f * scale;
        const auto stripeHeight = juce::jmin(46.0f * scale, rail.getHeight() - 40.0f * scale);
        const auto stripeY = rail.getY() + 24.0f * scale;
        graphics.setColour(stripe % 2 == 0 ? juce::Colour::fromRGB(242, 102, 53) : juce::Colour::fromRGB(237, 190, 72));
        graphics.fillRoundedRectangle(x, stripeY, 4.5f * scale, stripeHeight, 2.25f * scale);
    }
}
} // namespace

CompressorAudioProcessorEditor::CompressorAudioProcessorEditor(CompressorAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), inputMeter("IN", audioProcessor.getInputLevelSource()), outputMeter("OUT", audioProcessor.getOutputLevelSource()), reductionMeter("VU", audioProcessor.getInputLevelSource(), audioProcessor.getOutputLevelSource(), audioProcessor.getGainReductionSource())
{
    setLookAndFeel(&weatheredLookAndFeel);
    setResizable(true, true);
    setResizeLimits(minimumEditorWidth, minimumEditorHeight, maximumEditorWidth, maximumEditorHeight);
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio(designAspectRatio);

    configureChoiceControl(detectorModeLabel, detectorMode, "Detector", { "Vintage", "Fast" });
    configureChoiceControl(characterLabel, character, "Character", { "Clean", "Transformer", "FET Push" });
    configureChoiceControl(oversamplingLabel, oversampling, "Oversamp", { "Off", "2x", "4x" });
    for (auto* label : { &detectorModeLabel, &characterLabel, &oversamplingLabel })
        label->setColour(juce::Label::textColourId, juce::Colour::fromRGB(190, 202, 202));
    ratioLabel.setText("RATIO", juce::dontSendNotification);
    ratioLabel.setJustificationType(juce::Justification::centred);
    ratioLabel.setFont(juce::FontOptions(12.5f).withStyle("Bold"));
    ratioLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(34, 39, 43));
    ratioLabel.setInterceptsMouseClicks(false, false);

    for (auto* control : { &input, &attack, &release, &makeup, &mix })
        addAndMakeVisible(*control);

    addAndMakeVisible(ratioLabel);
    for (size_t index = 0; index < ratioButtons.size(); ++index)
    {
        configureRatioButton(ratioButtons[index], ratioButtonSpecs[index].text, ratioButtonSpecs[index].value);
        addAndMakeVisible(ratioButtons[index]);
    }

    for (size_t index = 0; index < meterModeButtons.size(); ++index)
    {
        meterModeButtons[index].setButtonText(meterModeButtonSpecs[index].text);
        meterModeButtons[index].setClickingTogglesState(false);
        meterModeButtons[index].setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(22, 26, 29));
        meterModeButtons[index].setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(237, 190, 72));
        meterModeButtons[index].setColour(juce::TextButton::textColourOffId, juce::Colour::fromRGB(218, 226, 223));
        meterModeButtons[index].setColour(juce::TextButton::textColourOnId, juce::Colour::fromRGB(21, 24, 27));
        meterModeButtons[index].onClick = [this, index] { setMeterMode(meterModeButtonSpecs[index].mode); };
        addAndMakeVisible(meterModeButtons[index]);
    }

    if (audioProcessor.getTier() == PluginTier::Deluxe)
    {
        addAndMakeVisible(knee);
        addAndMakeVisible(sidechainHighPass);
        for (auto* label : { &detectorModeLabel, &characterLabel, &oversamplingLabel })
            addAndMakeVisible(*label);
        for (auto* selector : { &detectorMode, &character, &oversampling })
            addAndMakeVisible(*selector);
    }

    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);
    addAndMakeVisible(reductionMeter);
    addAndMakeVisible(bypass);
    applyControlHierarchy();
    auto& state = audioProcessor.parameters;
    inputAttachment = std::make_unique<Attachment>(state, "input", input.slider);
    thresholdAttachment = std::make_unique<Attachment>(state, "threshold", threshold.slider);
    attackAttachment = std::make_unique<Attachment>(state, "attack", attack.slider);
    releaseAttachment = std::make_unique<Attachment>(state, "release", release.slider);
    makeupAttachment = std::make_unique<Attachment>(state, "makeup", makeup.slider);
    mixAttachment = std::make_unique<Attachment>(state, "mix", mix.slider);
    outputAttachment = std::make_unique<Attachment>(state, "output", output.slider);
    kneeAttachment = std::make_unique<Attachment>(state, "knee", knee.slider);
    highPassAttachment = std::make_unique<Attachment>(state, "sidechainHPF", sidechainHighPass.slider);
    detectorModeAttachment = std::make_unique<ComboAttachment>(state, "detectorMode", detectorMode);
    characterAttachment = std::make_unique<ComboAttachment>(state, "character", character);
    oversamplingAttachment = std::make_unique<ComboAttachment>(state, "oversampling", oversampling);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(state, "bypass", bypass);
    setMeterMode(MeterMode::GainReduction);
    updateRatioButtons();
    setSize(defaultEditorWidth, defaultEditorHeight);
    startTimerHz(30);
}
CompressorAudioProcessorEditor::~CompressorAudioProcessorEditor() { setLookAndFeel(nullptr); }

void CompressorAudioProcessorEditor::configureChoiceControl(juce::Label& label, juce::ComboBox& selector,
                                                            const juce::String& labelText, const juce::StringArray& items)
{
    label.setText(labelText.toUpperCase(), juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::FontOptions(12.0f).withStyle("Bold"));
    label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(34, 39, 43));
    label.setInterceptsMouseClicks(false, false);

    selector.setJustificationType(juce::Justification::centredLeft);
    selector.setScrollWheelEnabled(true);
    for (int item = 0; item < items.size(); ++item)
        selector.addItem(items[item], item + 1);
}

void CompressorAudioProcessorEditor::configureRatioButton(juce::TextButton& button, const juce::String& text, float ratioValue)
{
    button.setButtonText(text);
    button.setClickingTogglesState(false);
    button.setWantsKeyboardFocus(true);
    button.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(22, 26, 29));
    button.setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(242, 102, 53));
    button.setColour(juce::TextButton::textColourOffId, juce::Colour::fromRGB(218, 226, 223));
    button.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    button.onClick = [this, ratioValue] { setRatioFromButton(ratioValue); };
}

void CompressorAudioProcessorEditor::layoutChoiceControl(juce::Label& label, juce::ComboBox& selector, juce::Rectangle<int> bounds)
{
    const auto labelHeight = juce::jlimit(14, 18, bounds.getHeight() / 3);
    label.setBounds(bounds.removeFromTop(labelHeight));
    bounds.removeFromTop(2);
    selector.setBounds(bounds);
}

void CompressorAudioProcessorEditor::layoutRatioButtons(juce::Rectangle<int> bounds)
{
    const auto labelHeight = juce::jlimit(18, 24, bounds.getHeight() / 4);
    ratioLabel.setBounds(bounds.removeFromTop(labelHeight));
    bounds.removeFromTop(juce::jlimit(4, 8, bounds.getHeight() / 12));

    const auto gap = juce::jlimit(3, 5, bounds.getWidth() / 52);
    const auto buttonHeight = juce::jlimit(24, 34, bounds.getHeight() / 2);
    auto buttonRow = bounds.withHeight(buttonHeight).withCentre({ bounds.getCentreX(), bounds.getCentreY() });
    const auto buttonWidth = (buttonRow.getWidth() - gap * static_cast<int>(ratioButtons.size() - 1)) / static_cast<int>(ratioButtons.size());

    for (auto& button : ratioButtons)
    {
        button.setBounds(buttonRow.removeFromLeft(buttonWidth));
        buttonRow.removeFromLeft(gap);
    }
}

void CompressorAudioProcessorEditor::layoutMeterModeButtons(juce::Rectangle<int> bounds)
{
    const auto gap = juce::jlimit(4, 7, bounds.getWidth() / 28);
    const auto buttonWidth = (bounds.getWidth() - gap * static_cast<int>(meterModeButtons.size() - 1)) / static_cast<int>(meterModeButtons.size());

    for (auto& button : meterModeButtons)
    {
        button.setBounds(bounds.removeFromLeft(buttonWidth));
        bounds.removeFromLeft(gap);
    }
}

void CompressorAudioProcessorEditor::setRatioFromButton(float ratioValue)
{
    if (auto* parameter = audioProcessor.parameters.getParameter("ratio"))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(ratioValue));
        parameter->endChangeGesture();
    }

    updateRatioButtons();
}

void CompressorAudioProcessorEditor::setMeterMode(MeterMode newMode)
{
    meterMode = newMode;
    reductionMeter.setMeterMode(meterMode);
    updateMeterModeButtons();
}

void CompressorAudioProcessorEditor::updateMeterModeButtons()
{
    for (size_t index = 0; index < meterModeButtons.size(); ++index)
        meterModeButtons[index].setToggleState(meterModeButtonSpecs[index].mode == meterMode, juce::dontSendNotification);
}

void CompressorAudioProcessorEditor::updateRatioButtons()
{
    const auto selectedRatio = nearestRatioButtonValue(audioProcessor.parameters.getRawParameterValue("ratio")->load());

    for (size_t index = 0; index < ratioButtons.size(); ++index)
        ratioButtons[index].setToggleState(std::abs(ratioButtonSpecs[index].value - selectedRatio) < 0.01f, juce::dontSendNotification);
}

void CompressorAudioProcessorEditor::applyControlHierarchy()
{
    threshold.setControlSize(KnobComponent::Size::Large);
    makeup.setControlSize(KnobComponent::Size::Large);
    input.setControlSize(KnobComponent::Size::Large);
    output.setControlSize(KnobComponent::Size::Medium);
    attack.setControlSize(KnobComponent::Size::Small);
    release.setControlSize(KnobComponent::Size::Small);
    mix.setControlSize(KnobComponent::Size::Small);
    sidechainHighPass.setControlSize(KnobComponent::Size::Small);
    knee.setControlSize(KnobComponent::Size::Small);
}
void CompressorAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto scale = editorScaleFor(getWidth(), getHeight());
    const auto bounds = visualCanvasFor(getLocalBounds());
    const auto panelInset = 18.0f * scale;
    auto panel = bounds.reduced(panelInset);
    const auto fullPanel = panel;
    const auto bottomRailHeight = 92.0f * scale;
    const auto bottomRail = panel.removeFromBottom(bottomRailHeight);
    const auto compactRail = bottomRail.getWidth() < 760.0f;

    g.fillAll(juce::Colour::fromRGB(7, 9, 10));
    g.setColour(juce::Colour::fromRGB(0, 0, 0).withAlpha(0.34f));
    g.fillRoundedRectangle(fullPanel.translated(0.0f, 7.0f * scale), 9.0f * scale);

    juce::ColourGradient panelGradient(juce::Colour::fromRGB(235, 239, 238), fullPanel.getX(), fullPanel.getY(),
                                       juce::Colour::fromRGB(205, 213, 213), fullPanel.getRight(), fullPanel.getBottom(), false);
    g.setGradientFill(panelGradient);
    g.fillRoundedRectangle(fullPanel, 9.0f * scale);
    drawAgeMarks(g, fullPanel, scale);

    g.setColour(juce::Colour::fromRGB(18, 22, 25));
    g.fillRect(bottomRail);
    drawAgeMarks(g, bottomRail, scale);
    drawBottomRailGrime(g, bottomRail, scale);
    g.setColour(juce::Colour::fromRGB(77, 88, 94));
    g.drawHorizontalLine(static_cast<int>(bottomRail.getY()), bottomRail.getX(), bottomRail.getRight());

    const auto topBand = fullPanel.withHeight(84.0f * scale);
    g.setColour(juce::Colour::fromRGB(246, 248, 247).withAlpha(0.62f));
    g.fillRect(topBand);
    g.setColour(juce::Colour::fromRGB(173, 184, 187));
    g.drawHorizontalLine(static_cast<int>(topBand.getBottom()), fullPanel.getX(), fullPanel.getRight());

    g.setColour(juce::Colour::fromRGB(54, 64, 70));
    g.drawRoundedRectangle(fullPanel, 9.0f * scale, 1.3f * scale);

    const auto panelScrewInset = 24.0f * scale;
    const auto railScrewInset = 26.0f * scale;
    for (auto point : { juce::Point<float>(fullPanel.getX() + panelScrewInset, fullPanel.getY() + panelScrewInset),
                        juce::Point<float>(fullPanel.getRight() - panelScrewInset, fullPanel.getY() + panelScrewInset),
                        juce::Point<float>(panel.getX() + panelScrewInset, bottomRail.getY() - panelScrewInset),
                        juce::Point<float>(panel.getRight() - panelScrewInset, bottomRail.getY() - panelScrewInset),
                        juce::Point<float>(fullPanel.getX() + railScrewInset, bottomRail.getY() + 28.0f * scale),
                        juce::Point<float>(fullPanel.getRight() - railScrewInset, bottomRail.getY() + 28.0f * scale) })
        drawScrew(g, point, 11.0f * scale);

    const auto brandX = fullPanel.getX() + 68.0f * scale;
    g.setColour(juce::Colour::fromRGB(27, 32, 36));
    g.setFont(juce::FontOptions(27.0f * scale).withStyle("Bold"));
    g.drawText("808BYTES", static_cast<int>(brandX), static_cast<int>(fullPanel.getY() + 35.0f * scale),
               static_cast<int>(230.0f * scale), static_cast<int>(34.0f * scale), juce::Justification::centredLeft);
    g.setColour(juce::Colour::fromRGB(90, 101, 108));
    g.setFont(juce::FontOptions(10.5f * scale).withStyle("Bold"));
    g.drawText("FET COMPRESSOR", static_cast<int>(brandX), static_cast<int>(fullPanel.getY() + 64.0f * scale),
               static_cast<int>(210.0f * scale), static_cast<int>(18.0f * scale), juce::Justification::centredLeft);
    g.setColour(juce::Colour::fromRGB(242, 102, 53));
    g.fillRoundedRectangle(brandX, fullPanel.getY() + 91.0f * scale, 94.0f * scale, 4.0f * scale, 2.0f * scale);
    g.setColour(juce::Colour::fromRGB(237, 190, 72));
    g.fillRoundedRectangle(brandX + 102.0f * scale, fullPanel.getY() + 91.0f * scale, 38.0f * scale, 4.0f * scale, 2.0f * scale);

    if (! compactRail)
    {
        g.setColour(juce::Colour::fromRGB(211, 222, 222));
        g.setFont(juce::FontOptions(13.0f * scale).withStyle("Bold"));
        const auto analogArea = juce::Rectangle<float>(fullPanel.getX() + 90.0f * scale, bottomRail.getY() + 26.0f * scale,
                                                       120.0f * scale, 24.0f * scale);
        g.drawText("ANALOG PATH", analogArea.toNearestInt(), juce::Justification::centredLeft);

        const auto selectorLeft = audioProcessor.getTier() == PluginTier::Deluxe && detectorMode.isVisible()
            ? static_cast<float>(detectorMode.getX())
            : fullPanel.getRight();
        const auto presetAreaLeft = analogArea.getRight() + 24.0f * scale;
        const auto presetAreaRight = selectorLeft - 18.0f * scale;
        const auto presetAreaWidth = presetAreaRight - presetAreaLeft;

        if (presetAreaWidth >= 190.0f * scale)
        {
            const auto presetPlateWidth = juce::jlimit(190.0f * scale, 360.0f * scale, presetAreaWidth * 0.78f);
            auto presetPlate = juce::Rectangle<float>(presetPlateWidth, 36.0f * scale).withCentre({ presetAreaLeft + presetAreaWidth * 0.5f, bottomRail.getCentreY() });
            g.setColour(juce::Colour::fromRGB(11, 14, 16));
            g.fillRoundedRectangle(presetPlate, 5.0f * scale);
            g.setColour(juce::Colour::fromRGB(86, 100, 108));
            g.drawRoundedRectangle(presetPlate, 5.0f * scale, 1.1f * scale);
            g.setColour(juce::Colour::fromRGB(71, 211, 157));
            g.setFont(juce::FontOptions(13.5f * scale).withStyle("Bold"));
            g.drawText("76 FET", presetPlate.reduced(16.0f * scale, 0.0f).toNearestInt(), juce::Justification::centredLeft);
            g.setColour(juce::Colour::fromRGB(237, 190, 72));
            g.fillEllipse(presetPlate.getRight() - 25.0f * scale, presetPlate.getCentreY() - 4.0f * scale, 8.0f * scale, 8.0f * scale);
        }
    }

    for (const auto* knob : { &input, &makeup })
        drawKnobGrime(g, *knob, 0.70f);

    if (audioProcessor.getTier() == PluginTier::Deluxe)
    {
        for (const auto* knob : { &attack, &release, &mix, &knee, &sidechainHighPass })
            drawKnobGrime(g, *knob, 0.62f);
    }
    else
    {
        for (const auto* knob : { &attack, &release, &mix })
            drawKnobGrime(g, *knob, 0.62f);
    }
}
void CompressorAudioProcessorEditor::resized()
{
    const auto canvas = visualCanvasFor(getLocalBounds());
    const auto scale = canvas.getWidth() / designWidth;
    const auto labelScale = juce::jlimit(0.72f, 1.34f, scale);
    ratioLabel.setFont(juce::FontOptions(12.5f * labelScale).withStyle("Bold"));

    for (auto* label : { &detectorModeLabel, &characterLabel, &oversamplingLabel })
        label->setFont(juce::FontOptions(12.0f * labelScale).withStyle("Bold"));

    const auto designCanvas = juce::Rectangle<float>(designWidth, designHeight);
    const auto fullPanel = designCanvas.reduced(18.0f);
    auto body = fullPanel;
    body.removeFromBottom(92.0f);
    const auto bottomRail = fullPanel.withTrimmedTop(fullPanel.getHeight() - 92.0f);
    const auto toEditorBounds = [&canvas] (juce::Rectangle<float> designBounds)
    {
        return designToEditorBounds(canvas, designBounds);
    };

    const auto meterBounds = juce::Rectangle<float>(430.0f, 160.0f).withCentre({ designWidth * 0.5f, body.getY() + 122.0f });
    reductionMeter.setBounds(toEditorBounds(meterBounds));
    layoutMeterModeButtons(toEditorBounds(juce::Rectangle<float>(176.0f, 32.0f).withCentre({ meterBounds.getCentreX(), meterBounds.getBottom() + 16.0f })));

    constexpr auto levelMeterWidth = 54.0f;
    constexpr auto levelMeterGap = 18.0f;
    inputMeter.setBounds(toEditorBounds({ meterBounds.getX() - levelMeterGap - levelMeterWidth, meterBounds.getY(), levelMeterWidth, meterBounds.getHeight() }));
    outputMeter.setBounds(toEditorBounds({ meterBounds.getRight() + levelMeterGap, meterBounds.getY(), levelMeterWidth, meterBounds.getHeight() }));
    bypass.setBounds(toEditorBounds(juce::Rectangle<float>(92.0f, 92.0f).withCentre({ fullPanel.getRight() - 116.0f, body.getY() + 112.0f })));

    constexpr auto smallWidth = 132.0f;
    constexpr auto smallHeight = 144.0f;
    constexpr auto largeWidth = 226.0f;
    constexpr auto largeHeight = 236.0f;
    const auto smallY = body.getBottom() - smallHeight - 14.0f;
    const auto largeY = meterBounds.getBottom() + 18.0f;

    const std::array<std::pair<KnobComponent*, float>, 2> dominantControls {{
        { &input, 0.34f },
        { &makeup, 0.66f }
    }};

    for (const auto& [control, xRatio] : dominantControls)
    {
        const auto centreX = body.getX() + body.getWidth() * xRatio;
        control->setBounds(toEditorBounds(juce::Rectangle<float>(largeWidth, largeHeight).withCentre({ centreX, largeY + largeHeight * 0.5f })));
    }

    threshold.setBounds({});
    output.setBounds({});

    const auto layoutSecondaryControl = [&body, &toEditorBounds, smallY] (KnobComponent& control, float xRatio)
    {
        const auto centreX = body.getX() + body.getWidth() * xRatio;
        control.setBounds(toEditorBounds(juce::Rectangle<float>(smallWidth, smallHeight).withCentre({ centreX, smallY + smallHeight * 0.5f })));
    };

    if (audioProcessor.getTier() == PluginTier::Deluxe)
    {
        layoutRatioButtons(toEditorBounds(juce::Rectangle<float>(260.0f, 102.0f).withCentre({
            body.getX() + body.getWidth() * 0.16f,
            smallY + smallHeight * 0.5f
        })));

        const std::array<std::pair<KnobComponent*, float>, 5> secondaryControls {{
            { &attack, 0.36f },
            { &release, 0.49f },
            { &mix, 0.62f },
            { &knee, 0.74f },
            { &sidechainHighPass, 0.86f }
        }};

        for (const auto& [control, xRatio] : secondaryControls)
            layoutSecondaryControl(*control, xRatio);

        constexpr auto selectorWidth = 104.0f;
        constexpr auto selectorGap = 10.0f;
        const auto selectorGroupWidth = selectorWidth * 3 + selectorGap * 2;
        constexpr auto selectorHeight = 52.0f;
        const auto selectorX = fullPanel.getRight() - 30.0f - selectorGroupWidth;
        const auto selectorY = bottomRail.getBottom() - selectorHeight - 12.0f;
        layoutChoiceControl(detectorModeLabel, detectorMode, toEditorBounds({ selectorX, selectorY, selectorWidth, selectorHeight }));
        layoutChoiceControl(characterLabel, character, toEditorBounds({ selectorX + selectorWidth + selectorGap, selectorY, selectorWidth, selectorHeight }));
        layoutChoiceControl(oversamplingLabel, oversampling, toEditorBounds({ selectorX + (selectorWidth + selectorGap) * 2.0f, selectorY, selectorWidth, selectorHeight }));
    }
    else
    {
        layoutRatioButtons(toEditorBounds(juce::Rectangle<float>(260.0f, 102.0f).withCentre({
            body.getX() + body.getWidth() * 0.25f,
            smallY + smallHeight * 0.5f
        })));

        const std::array<std::pair<KnobComponent*, float>, 3> secondaryControls {{
            { &attack, 0.49f },
            { &release, 0.65f },
            { &mix, 0.81f }
        }};

        for (const auto& [control, xRatio] : secondaryControls)
            layoutSecondaryControl(*control, xRatio);
    }
}
void CompressorAudioProcessorEditor::timerCallback()
{
    inputMeter.repaint();
    outputMeter.repaint();
    reductionMeter.repaint();
    updateRatioButtons();
    updateMeterModeButtons();
}
} // namespace compressor808bytes
