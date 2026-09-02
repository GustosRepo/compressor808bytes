#include "PluginEditor.h"

#include <array>
#include <cmath>

namespace compressor808bytes
{
namespace
{
float editorScaleFor(int width, int height) noexcept
{
    return juce::jlimit(0.62f, 1.0f, std::min(static_cast<float>(width) / 1120.0f, static_cast<float>(height) / 760.0f));
}

void drawScrew(juce::Graphics& graphics, juce::Point<float> centre, float radius)
{
    const auto bounds = juce::Rectangle<float>(radius * 2.0f, radius * 2.0f).withCentre(centre);
    graphics.setColour(juce::Colour::fromRGB(86, 48, 22).withAlpha(0.22f));
    graphics.fillEllipse(bounds.expanded(radius * 0.76f).translated(1.0f, 2.0f));
    graphics.setColour(juce::Colour::fromRGB(28, 18, 11).withAlpha(0.18f));
    graphics.fillEllipse(bounds.expanded(radius * 0.38f).translated(-1.0f, 1.0f));
    graphics.setColour(juce::Colour::fromRGB(7, 7, 6));
    graphics.fillEllipse(bounds.translated(1.0f, 2.0f));
    graphics.setColour(juce::Colour::fromRGB(31, 28, 23));
    graphics.fillEllipse(bounds);
    graphics.setColour(juce::Colour::fromRGB(80, 72, 58));
    graphics.drawEllipse(bounds.reduced(1.0f), 1.0f);
    graphics.setColour(juce::Colour::fromRGB(3, 3, 3));
    graphics.drawLine(centre.x - radius * 0.48f, centre.y, centre.x + radius * 0.48f, centre.y, 2.5f);
    graphics.drawLine(centre.x, centre.y - radius * 0.48f, centre.x, centre.y + radius * 0.48f, 2.5f);
    graphics.setColour(juce::Colour::fromRGB(124, 71, 30).withAlpha(0.42f));
    const auto rustArc = bounds.expanded(2.0f);
    juce::Path rust;
    rust.addArc(rustArc.getX(), rustArc.getY(), rustArc.getWidth(), rustArc.getHeight(), 0.12f, 1.36f, true);
    graphics.strokePath(rust, juce::PathStrokeType(1.5f));
}

void drawAgeMarks(juce::Graphics& graphics, juce::Rectangle<float> area)
{
    for (int mark = 0; mark < 620; ++mark)
    {
        const auto x = area.getX() + static_cast<float>((mark * 71) % juce::jmax(1, static_cast<int>(area.getWidth())));
        const auto y = area.getY() + static_cast<float>((mark * 43) % juce::jmax(1, static_cast<int>(area.getHeight())));
        const auto size = 0.8f + static_cast<float>((mark * 17) % 7);
        graphics.setColour(juce::Colour::fromRGB(54, 43, 29).withAlpha(mark % 9 == 0 ? 0.24f : 0.085f));
        graphics.fillEllipse(x, y, size, size * 0.62f);
    }

    for (int scratch = 0; scratch < 58; ++scratch)
    {
        const auto x = area.getX() + static_cast<float>((scratch * 107) % juce::jmax(1, static_cast<int>(area.getWidth())));
        const auto y = area.getY() + static_cast<float>((scratch * 61) % juce::jmax(1, static_cast<int>(area.getHeight())));
        const auto length = 18.0f + static_cast<float>(scratch % 7) * 11.0f;
        const auto slope = -10.0f + static_cast<float>(scratch % 5) * 5.0f;
        graphics.setColour(juce::Colour::fromRGB(35, 27, 18).withAlpha(scratch % 4 == 0 ? 0.34f : 0.22f));
        graphics.drawLine(x, y, x + length, y + slope, scratch % 6 == 0 ? 1.3f : 0.8f);
        graphics.setColour(juce::Colours::white.withAlpha(0.045f));
        graphics.drawLine(x + 1.0f, y - 1.0f, x + length + 1.0f, y + slope - 1.0f, 0.45f);
    }
}

void drawEdgeWear(juce::Graphics& graphics, juce::Rectangle<float> area)
{
    graphics.setColour(juce::Colour::fromRGB(24, 19, 14).withAlpha(0.23f));
    graphics.fillRect(area.withTrimmedBottom(area.getHeight() - 26.0f));
    graphics.fillRect(area.withTrimmedTop(area.getHeight() - 28.0f));
    graphics.fillRect(area.withTrimmedRight(area.getWidth() - 24.0f));
    graphics.fillRect(area.withTrimmedLeft(area.getWidth() - 24.0f));

    for (int chip = 0; chip < 54; ++chip)
    {
        const auto horizontal = chip % 2 == 0;
        const auto x = horizontal
            ? area.getX() + static_cast<float>((chip * 83) % juce::jmax(1, static_cast<int>(area.getWidth())))
            : (chip % 4 == 1 ? area.getX() + 3.0f : area.getRight() - 15.0f);
        const auto y = horizontal
            ? (chip % 4 == 0 ? area.getY() + 3.0f : area.getBottom() - 12.0f)
            : area.getY() + static_cast<float>((chip * 67) % juce::jmax(1, static_cast<int>(area.getHeight())));
        const auto width = horizontal ? 11.0f + static_cast<float>((chip * 5) % 25) : 8.0f + static_cast<float>((chip * 3) % 9);
        const auto height = horizontal ? 5.0f + static_cast<float>((chip * 7) % 7) : 12.0f + static_cast<float>((chip * 5) % 19);

        graphics.setColour(juce::Colour::fromRGB(34, 27, 20).withAlpha(0.42f));
        graphics.fillRoundedRectangle(x, y, width, height, 2.0f);
        graphics.setColour(juce::Colour::fromRGB(211, 197, 163).withAlpha(0.13f));
        graphics.drawLine(x + 1.0f, y + 1.0f, x + width - 2.0f, y + 1.0f, 0.65f);
    }
}

void drawGrimeAround(juce::Graphics& graphics, juce::Point<float> centre, float radius)
{
    graphics.setColour(juce::Colour::fromRGB(31, 22, 14).withAlpha(0.13f));
    graphics.fillEllipse(juce::Rectangle<float>(radius * 2.0f, radius * 1.18f).withCentre(centre).translated(0.0f, radius * 0.12f));
    graphics.setColour(juce::Colour::fromRGB(98, 56, 24).withAlpha(0.08f));
    graphics.fillEllipse(juce::Rectangle<float>(radius * 1.55f, radius * 0.82f).withCentre(centre).translated(-radius * 0.15f, radius * 0.2f));
}

void drawKnobGrime(juce::Graphics& graphics, const KnobComponent& knob, float multiplier)
{
    const auto knobBounds = knob.getBounds().toFloat();
    const auto sliderBounds = knob.slider.getBounds().toFloat().translated(knobBounds.getX(), knobBounds.getY());
    const auto radius = std::min(sliderBounds.getWidth(), sliderBounds.getHeight()) * multiplier;
    drawGrimeAround(graphics, sliderBounds.getCentre(), radius);
}

void drawBottomRailGrime(juce::Graphics& graphics, juce::Rectangle<float> rail)
{
    graphics.setColour(juce::Colour::fromRGB(6, 5, 4).withAlpha(0.38f));
    graphics.fillRect(rail.withTrimmedTop(rail.getHeight() * 0.58f));

    for (int streak = 0; streak < 18; ++streak)
    {
        const auto x = rail.getX() + static_cast<float>((streak * 79) % juce::jmax(1, static_cast<int>(rail.getWidth())));
        const auto y = rail.getY() + 8.0f + static_cast<float>((streak * 17) % juce::jmax(1, static_cast<int>(rail.getHeight() - 18.0f)));
        graphics.setColour(juce::Colour::fromRGB(108, 65, 31).withAlpha(0.16f));
        graphics.drawLine(x, y, x + 4.0f + static_cast<float>(streak % 4), y + 26.0f + static_cast<float>((streak * 3) % 30), 1.2f);
    }
}
} // namespace

CompressorAudioProcessorEditor::CompressorAudioProcessorEditor(CompressorAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), inputMeter("IN", audioProcessor.getInputLevelSource()), outputMeter("OUT", audioProcessor.getOutputLevelSource()), reductionMeter("GR", audioProcessor.getGainReductionSource(), true)
{
    setLookAndFeel(&weatheredLookAndFeel);
    setResizable(true, true);
    setResizeLimits(680, 440, 1500, 980);
    configureChoiceControl(detectorModeLabel, detectorMode, "Detector", { "RMS", "Peak" });
    configureChoiceControl(characterLabel, character, "Character", { "Clean", "Warm", "Punch" });
    configureChoiceControl(oversamplingLabel, oversampling, "Oversamp", { "Off", "2x", "4x" });

    for (auto* control : { &input, &threshold, &ratio, &attack, &release, &makeup, &mix, &output })
        addAndMakeVisible(*control);

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
    ratioAttachment = std::make_unique<Attachment>(state, "ratio", ratio.slider);
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
    setSize(820, 540);
    startTimerHz(30);
}
CompressorAudioProcessorEditor::~CompressorAudioProcessorEditor() { setLookAndFeel(nullptr); }

void CompressorAudioProcessorEditor::configureChoiceControl(juce::Label& label, juce::ComboBox& selector,
                                                            const juce::String& labelText, const juce::StringArray& items)
{
    label.setText(labelText.toUpperCase(), juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::FontOptions(12.0f).withStyle("Bold"));
    label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(37, 31, 24));
    label.setInterceptsMouseClicks(false, false);

    selector.setJustificationType(juce::Justification::centredLeft);
    selector.setScrollWheelEnabled(true);
    for (int item = 0; item < items.size(); ++item)
        selector.addItem(items[item], item + 1);
}

void CompressorAudioProcessorEditor::layoutChoiceControl(juce::Label& label, juce::ComboBox& selector, juce::Rectangle<int> bounds)
{
    const auto labelHeight = juce::jlimit(14, 18, bounds.getHeight() / 3);
    label.setBounds(bounds.removeFromTop(labelHeight));
    bounds.removeFromTop(2);
    selector.setBounds(bounds);
}

void CompressorAudioProcessorEditor::applyControlHierarchy()
{
    threshold.setControlSize(KnobComponent::Size::Large);
    makeup.setControlSize(KnobComponent::Size::Large);
    input.setControlSize(KnobComponent::Size::Medium);
    output.setControlSize(KnobComponent::Size::Medium);
    ratio.setControlSize(KnobComponent::Size::Small);
    attack.setControlSize(KnobComponent::Size::Small);
    release.setControlSize(KnobComponent::Size::Small);
    mix.setControlSize(KnobComponent::Size::Small);
    sidechainHighPass.setControlSize(KnobComponent::Size::Small);
    knee.setControlSize(KnobComponent::Size::Small);
}
void CompressorAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto scale = editorScaleFor(getWidth(), getHeight());
    const auto bounds = getLocalBounds().toFloat();
    const auto panelInset = 18.0f * scale;
    auto panel = bounds.reduced(panelInset);
    const auto fullPanel = panel;
    const auto bottomRailHeight = 92.0f * scale;
    const auto bottomRail = panel.removeFromBottom(bottomRailHeight);

    g.fillAll(juce::Colour::fromRGB(10, 10, 9));
    g.setColour(juce::Colour::fromRGB(10, 10, 9));
    g.fillRoundedRectangle(fullPanel.expanded(4.0f), 8.0f);
    g.setColour(juce::Colour::fromRGB(188, 181, 162));
    g.fillRoundedRectangle(fullPanel, 6.0f);
    drawAgeMarks(g, fullPanel);
    drawEdgeWear(g, fullPanel);

    g.setColour(juce::Colour::fromRGB(17, 16, 14));
    g.fillRect(bottomRail);
    drawAgeMarks(g, bottomRail);
    drawBottomRailGrime(g, bottomRail);
    g.setColour(juce::Colour::fromRGB(49, 40, 29));
    g.drawHorizontalLine(static_cast<int>(bottomRail.getY()), bottomRail.getX(), bottomRail.getRight());

    g.setColour(juce::Colour::fromRGB(37, 31, 24));
    g.drawRoundedRectangle(fullPanel, 6.0f, 2.0f);
    g.drawRect(bottomRail.toNearestInt(), 1);

    const auto panelScrewInset = 24.0f * scale;
    const auto railScrewInset = 26.0f * scale;
    for (auto point : { juce::Point<float>(fullPanel.getX() + panelScrewInset, fullPanel.getY() + panelScrewInset),
                        juce::Point<float>(fullPanel.getRight() - panelScrewInset, fullPanel.getY() + panelScrewInset),
                        juce::Point<float>(panel.getX() + panelScrewInset, bottomRail.getY() - panelScrewInset),
                        juce::Point<float>(panel.getRight() - panelScrewInset, bottomRail.getY() - panelScrewInset),
                        juce::Point<float>(fullPanel.getX() + railScrewInset, bottomRail.getY() + 28.0f * scale),
                        juce::Point<float>(fullPanel.getRight() - railScrewInset, bottomRail.getY() + 28.0f * scale) })
        drawScrew(g, point, 11.0f * scale);

    g.setColour(juce::Colour::fromRGB(37, 31, 24));
    g.setFont(juce::FontOptions(27.0f * scale).withStyle("Bold"));
    g.drawText("808BYTES", static_cast<int>(fullPanel.getX() + 68.0f * scale), static_cast<int>(fullPanel.getY() + 52.0f * scale),
               static_cast<int>(230.0f * scale), static_cast<int>(34.0f * scale), juce::Justification::centredLeft);
    g.drawHorizontalLine(static_cast<int>(fullPanel.getY() + 100.0f * scale), fullPanel.getX() + 68.0f * scale, fullPanel.getX() + 270.0f * scale);

    const auto presetPlateWidth = juce::jlimit(210.0f * scale, 360.0f * scale, static_cast<float>(getWidth()) * 0.30f);
    auto presetPlate = juce::Rectangle<float>(presetPlateWidth, 36.0f * scale).withCentre({ static_cast<float>(getWidth()) * 0.5f, bottomRail.getCentreY() });
    g.setColour(juce::Colour::fromRGB(8, 8, 7));
    g.fillRoundedRectangle(presetPlate, 4.0f);
    g.setColour(juce::Colour::fromRGB(85, 70, 48));
    g.drawRoundedRectangle(presetPlate, 4.0f, 1.5f);
    drawScrew(g, { presetPlate.getX() + 16.0f * scale, presetPlate.getCentreY() }, 7.0f * scale);
    drawScrew(g, { presetPlate.getRight() - 16.0f * scale, presetPlate.getCentreY() }, 7.0f * scale);
    g.setColour(juce::Colour::fromRGB(168, 143, 101));
    g.setFont(juce::FontOptions(18.0f * scale));
    g.drawText("808Bytes", presetPlate.toNearestInt(), juce::Justification::centred);
    g.setColour(juce::Colour::fromRGB(58, 40, 21).withAlpha(0.36f));
    g.drawLine(presetPlate.getX() + 44.0f * scale, presetPlate.getY() + 9.0f * scale, presetPlate.getRight() - 54.0f * scale, presetPlate.getY() + 13.0f * scale, 1.0f);
    g.drawLine(presetPlate.getX() + 64.0f * scale, presetPlate.getBottom() - 8.0f * scale, presetPlate.getRight() - 38.0f * scale, presetPlate.getBottom() - 12.0f * scale, 0.8f);

    g.setColour(juce::Colour::fromRGB(170, 151, 113));
    g.setFont(juce::FontOptions(15.0f * scale));
    g.drawText("ANALOG", static_cast<int>(fullPanel.getX() + 90.0f * scale), static_cast<int>(bottomRail.getY() + 26.0f * scale),
               static_cast<int>(100.0f * scale), static_cast<int>(24.0f * scale), juce::Justification::centredLeft);

    for (const auto* knob : { &input, &threshold, &makeup, &output })
        drawKnobGrime(g, *knob, 0.70f);

    if (audioProcessor.getTier() == PluginTier::Deluxe)
    {
        for (const auto* knob : { &ratio, &attack, &release, &mix, &knee, &sidechainHighPass })
            drawKnobGrime(g, *knob, 0.62f);
    }
    else
    {
        for (const auto* knob : { &ratio, &attack, &release, &mix })
            drawKnobGrime(g, *knob, 0.62f);
    }
}
void CompressorAudioProcessorEditor::resized()
{
    const auto width = getWidth();
    const auto height = getHeight();
    const auto scale = editorScaleFor(width, height);
    const auto panelInset = juce::roundToInt(18.0f * scale);
    const auto bottomRailHeight = juce::roundToInt(92.0f * scale);
    auto fullPanel = getLocalBounds().reduced(panelInset);
    auto body = fullPanel;
    body.removeFromBottom(bottomRailHeight);

    const auto meterWidth = juce::jlimit(230, 430, juce::roundToInt(static_cast<float>(body.getWidth()) * 0.42f));
    const auto meterHeight = juce::jlimit(96, 160, juce::roundToInt(160.0f * scale));
    reductionMeter.setBounds(width / 2 - meterWidth / 2, body.getY() + juce::roundToInt(42.0f * scale), meterWidth, meterHeight);
    const auto levelMeterWidth = juce::jlimit(34, 54, juce::roundToInt(54.0f * scale));
    const auto levelMeterGap = juce::jlimit(10, 18, juce::roundToInt(18.0f * scale));
    inputMeter.setBounds(reductionMeter.getX() - levelMeterGap - levelMeterWidth, reductionMeter.getY(), levelMeterWidth, reductionMeter.getHeight());
    outputMeter.setBounds(reductionMeter.getRight() + levelMeterGap, reductionMeter.getY(), levelMeterWidth, reductionMeter.getHeight());
    const auto bypassSize = juce::jlimit(56, 92, juce::roundToInt(92.0f * scale));
    bypass.setBounds(fullPanel.getRight() - juce::roundToInt(116.0f * scale) - bypassSize / 2,
                     body.getY() + juce::roundToInt(66.0f * scale), bypassSize, bypassSize);

    const auto smallWidth = juce::jlimit(74, 132, body.getWidth() / (audioProcessor.getTier() == PluginTier::Deluxe ? 7 : 6));
    const auto smallHeight = juce::jlimit(88, 146, juce::roundToInt(static_cast<float>(height) * 0.19f));
    const auto smallY = body.getBottom() - smallHeight - juce::roundToInt(14.0f * scale);
    const auto meterBottom = reductionMeter.getBottom();
    const auto mediumWidth = juce::jlimit(96, 162, juce::roundToInt(static_cast<float>(body.getWidth()) * 0.145f));
    const auto mediumHeight = juce::jlimit(108, 174, smallY - meterBottom - juce::roundToInt(40.0f * scale));
    const auto largeWidth = juce::jlimit(138, 226, juce::roundToInt(static_cast<float>(body.getWidth()) * 0.24f));
    const auto largeHeight = juce::jlimit(126, 236, smallY - meterBottom - juce::roundToInt(18.0f * scale));
    const auto largeY = meterBottom + juce::roundToInt(18.0f * scale);

    const std::array<std::pair<KnobComponent*, float>, 2> dominantControls {{
        { &threshold, 0.34f },
        { &makeup, 0.66f }
    }};

    for (const auto& [control, xRatio] : dominantControls)
    {
        const auto centreX = body.getX() + juce::roundToInt(static_cast<float>(body.getWidth()) * xRatio);
        control->setBounds(juce::Rectangle<int>(largeWidth, largeHeight).withCentre({ centreX, largeY + largeHeight / 2 }));
    }

    const auto mediumY = largeY + (largeHeight - mediumHeight) / 2;
    input.setBounds(juce::Rectangle<int>(mediumWidth, mediumHeight).withCentre({ body.getX() + juce::roundToInt(static_cast<float>(body.getWidth()) * 0.13f), mediumY + mediumHeight / 2 }));
    output.setBounds(juce::Rectangle<int>(mediumWidth, mediumHeight).withCentre({ body.getX() + juce::roundToInt(static_cast<float>(body.getWidth()) * 0.87f), mediumY + mediumHeight / 2 }));

    const auto layoutSecondaryControl = [&body, smallWidth, smallHeight, smallY] (KnobComponent& control, float xRatio)
    {
        const auto centreX = body.getX() + juce::roundToInt(static_cast<float>(body.getWidth()) * xRatio);
        control.setBounds(juce::Rectangle<int>(smallWidth, smallHeight).withCentre({ centreX, smallY + smallHeight / 2 }));
    };

    if (audioProcessor.getTier() == PluginTier::Deluxe)
    {
        const std::array<std::pair<KnobComponent*, float>, 6> secondaryControls {{
            { &ratio, 0.10f },
            { &attack, 0.25f },
            { &release, 0.40f },
            { &mix, 0.55f },
            { &knee, 0.70f },
            { &sidechainHighPass, 0.85f }
        }};

        for (const auto& [control, xRatio] : secondaryControls)
            layoutSecondaryControl(*control, xRatio);

        const auto selectorWidth = juce::jlimit(68, 104, juce::roundToInt(static_cast<float>(fullPanel.getWidth()) * 0.105f));
        const auto selectorGap = juce::jlimit(7, 10, juce::roundToInt(10.0f * scale));
        const auto selectorGroupWidth = selectorWidth * 3 + selectorGap * 2;
        const auto selectorHeight = juce::jlimit(42, 52, juce::roundToInt(52.0f * scale));
        const auto selectorX = fullPanel.getRight() - juce::roundToInt(30.0f * scale) - selectorGroupWidth;
        const auto selectorY = fullPanel.getBottom() - selectorHeight - juce::roundToInt(12.0f * scale);
        layoutChoiceControl(detectorModeLabel, detectorMode, { selectorX, selectorY, selectorWidth, selectorHeight });
        layoutChoiceControl(characterLabel, character, { selectorX + selectorWidth + selectorGap, selectorY, selectorWidth, selectorHeight });
        layoutChoiceControl(oversamplingLabel, oversampling, { selectorX + (selectorWidth + selectorGap) * 2, selectorY, selectorWidth, selectorHeight });
    }
    else
    {
        const std::array<std::pair<KnobComponent*, float>, 4> secondaryControls {{
            { &ratio, 0.23f },
            { &attack, 0.41f },
            { &release, 0.59f },
            { &mix, 0.77f }
        }};

        for (const auto& [control, xRatio] : secondaryControls)
            layoutSecondaryControl(*control, xRatio);
    }
}
void CompressorAudioProcessorEditor::timerCallback() { inputMeter.repaint(); outputMeter.repaint(); reductionMeter.repaint(); }
} // namespace compressor808bytes
