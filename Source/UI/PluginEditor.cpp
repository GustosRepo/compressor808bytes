#include "PluginEditor.h"

#include <array>
#include <cmath>

namespace compressor808bytes
{
namespace
{
void drawScrew(juce::Graphics& graphics, juce::Point<float> centre, float radius)
{
    const auto bounds = juce::Rectangle<float>(radius * 2.0f, radius * 2.0f).withCentre(centre);
    graphics.setColour(juce::Colour::fromRGB(7, 7, 6));
    graphics.fillEllipse(bounds.translated(1.0f, 2.0f));
    graphics.setColour(juce::Colour::fromRGB(31, 28, 23));
    graphics.fillEllipse(bounds);
    graphics.setColour(juce::Colour::fromRGB(80, 72, 58));
    graphics.drawEllipse(bounds.reduced(1.0f), 1.0f);
    graphics.setColour(juce::Colour::fromRGB(3, 3, 3));
    graphics.drawLine(centre.x - radius * 0.48f, centre.y, centre.x + radius * 0.48f, centre.y, 2.5f);
    graphics.drawLine(centre.x, centre.y - radius * 0.48f, centre.x, centre.y + radius * 0.48f, 2.5f);
}

void drawAgeMarks(juce::Graphics& graphics, juce::Rectangle<float> area)
{
    for (int mark = 0; mark < 360; ++mark)
    {
        const auto x = area.getX() + static_cast<float>((mark * 71) % juce::jmax(1, static_cast<int>(area.getWidth())));
        const auto y = area.getY() + static_cast<float>((mark * 43) % juce::jmax(1, static_cast<int>(area.getHeight())));
        const auto size = 1.0f + static_cast<float>((mark * 17) % 5);
        graphics.setColour(juce::Colour::fromRGB(54, 43, 29).withAlpha(mark % 7 == 0 ? 0.18f : 0.075f));
        graphics.fillEllipse(x, y, size, size * 0.62f);
    }

    for (int scratch = 0; scratch < 24; ++scratch)
    {
        const auto x = area.getX() + static_cast<float>((scratch * 107) % juce::jmax(1, static_cast<int>(area.getWidth())));
        const auto y = area.getY() + static_cast<float>((scratch * 61) % juce::jmax(1, static_cast<int>(area.getHeight())));
        graphics.setColour(juce::Colour::fromRGB(67, 53, 37).withAlpha(0.22f));
        graphics.drawLine(x, y, x + 18.0f + static_cast<float>(scratch % 5) * 9.0f, y - 8.0f + static_cast<float>(scratch % 3) * 7.0f, 0.8f);
    }
}
} // namespace

CompressorAudioProcessorEditor::CompressorAudioProcessorEditor(CompressorAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), inputMeter("IN", audioProcessor.getInputLevelSource()), outputMeter("OUT", audioProcessor.getOutputLevelSource()), reductionMeter("GR", audioProcessor.getMeterGainChangeSource(), true)
{
    setLookAndFeel(&weatheredLookAndFeel);
    setResizable(true, true);
    setResizeLimits(980, 680, 1500, 980);
    for (auto* control : { &input, &threshold, &ratio, &attack, &release, &makeup, &mix, &output }) addAndMakeVisible(*control);
    if (audioProcessor.getTier() == PluginTier::Deluxe) { addChildComponent(knee); addAndMakeVisible(sidechainHighPass); }
    addAndMakeVisible(reductionMeter);
    addAndMakeVisible(bypass);
    applyControlHierarchy();
    auto& state = audioProcessor.parameters;
    inputAttachment = std::make_unique<Attachment>(state, "input", input.slider); thresholdAttachment = std::make_unique<Attachment>(state, "threshold", threshold.slider); ratioAttachment = std::make_unique<Attachment>(state, "ratio", ratio.slider); attackAttachment = std::make_unique<Attachment>(state, "attack", attack.slider); releaseAttachment = std::make_unique<Attachment>(state, "release", release.slider); makeupAttachment = std::make_unique<Attachment>(state, "makeup", makeup.slider); mixAttachment = std::make_unique<Attachment>(state, "mix", mix.slider); outputAttachment = std::make_unique<Attachment>(state, "output", output.slider); kneeAttachment = std::make_unique<Attachment>(state, "knee", knee.slider); highPassAttachment = std::make_unique<Attachment>(state, "sidechainHPF", sidechainHighPass.slider); bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(state, "bypass", bypass);
    setSize(1120, 760);
    startTimerHz(30);
}
CompressorAudioProcessorEditor::~CompressorAudioProcessorEditor() { setLookAndFeel(nullptr); }
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
    knee.setVisible(false);
}
void CompressorAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    auto panel = bounds.reduced(18.0f);
    const auto fullPanel = panel;
    const auto bottomRailHeight = 92.0f;
    const auto bottomRail = panel.removeFromBottom(bottomRailHeight);

    g.fillAll(juce::Colour::fromRGB(10, 10, 9));
    g.setColour(juce::Colour::fromRGB(10, 10, 9));
    g.fillRoundedRectangle(fullPanel.expanded(4.0f), 8.0f);
    g.setColour(juce::Colour::fromRGB(188, 181, 162));
    g.fillRoundedRectangle(fullPanel, 6.0f);
    drawAgeMarks(g, fullPanel);

    g.setColour(juce::Colour::fromRGB(17, 16, 14));
    g.fillRect(bottomRail);
    drawAgeMarks(g, bottomRail);
    g.setColour(juce::Colour::fromRGB(49, 40, 29));
    g.drawHorizontalLine(static_cast<int>(bottomRail.getY()), bottomRail.getX(), bottomRail.getRight());

    g.setColour(juce::Colour::fromRGB(37, 31, 24));
    g.drawRoundedRectangle(fullPanel, 6.0f, 2.0f);
    g.drawRect(bottomRail.toNearestInt(), 1);

    for (auto point : { juce::Point<float>(fullPanel.getX() + 24.0f, fullPanel.getY() + 24.0f),
                        juce::Point<float>(fullPanel.getRight() - 24.0f, fullPanel.getY() + 24.0f),
                        juce::Point<float>(panel.getX() + 24.0f, bottomRail.getY() - 24.0f),
                        juce::Point<float>(panel.getRight() - 24.0f, bottomRail.getY() - 24.0f),
                        juce::Point<float>(fullPanel.getX() + 26.0f, bottomRail.getY() + 28.0f),
                        juce::Point<float>(fullPanel.getRight() - 26.0f, bottomRail.getY() + 28.0f) })
        drawScrew(g, point, 11.0f);

    g.setColour(juce::Colour::fromRGB(37, 31, 24));
    g.setFont(juce::FontOptions(27.0f).withStyle("Bold"));
    g.drawText("808BYTES", 86, 70, 230, 34, juce::Justification::centredLeft);
    g.drawHorizontalLine(118, 86.0f, 288.0f);

    auto presetPlate = juce::Rectangle<float>(static_cast<float>(getWidth()) * 0.5f - 180.0f, bottomRail.getCentreY() - 18.0f, 360.0f, 36.0f);
    g.setColour(juce::Colour::fromRGB(8, 8, 7));
    g.fillRoundedRectangle(presetPlate, 4.0f);
    g.setColour(juce::Colour::fromRGB(85, 70, 48));
    g.drawRoundedRectangle(presetPlate, 4.0f, 1.5f);
    drawScrew(g, { presetPlate.getX() + 16.0f, presetPlate.getCentreY() }, 7.0f);
    drawScrew(g, { presetPlate.getRight() - 16.0f, presetPlate.getCentreY() }, 7.0f);
    g.setColour(juce::Colour::fromRGB(168, 143, 101));
    g.setFont(juce::FontOptions(18.0f));
    g.drawText("808Bytes", presetPlate.toNearestInt(), juce::Justification::centred);

    g.setColour(juce::Colour::fromRGB(170, 151, 113));
    g.setFont(juce::FontOptions(15.0f));
    g.drawText("ANALOG", static_cast<int>(fullPanel.getX() + 90.0f), static_cast<int>(bottomRail.getY() + 26.0f), 100, 24, juce::Justification::centredLeft);
}
void CompressorAudioProcessorEditor::resized()
{
    const auto width = getWidth();
    const auto height = getHeight();
    auto fullPanel = getLocalBounds().reduced(18);
    auto body = fullPanel;
    body.removeFromBottom(92);

    const auto meterWidth = juce::jlimit(330, 430, width - 610);
    reductionMeter.setBounds(width / 2 - meterWidth / 2, body.getY() + 42, meterWidth, 160);
    bypass.setBounds(width - 190, body.getY() + 66, 92, 92);

    const auto smallWidth = juce::jlimit(112, 132, body.getWidth() / 8);
    const auto smallHeight = juce::jlimit(126, 146, height / 5);
    const auto smallY = body.getBottom() - smallHeight - 18;
    const auto meterBottom = reductionMeter.getBottom();
    const auto mediumWidth = juce::jlimit(138, 162, body.getWidth() / 6);
    const auto mediumHeight = juce::jlimit(154, 174, smallY - meterBottom - 72);
    const auto largeWidth = juce::jlimit(184, 226, body.getWidth() / 4);
    const auto largeHeight = juce::jlimit(196, 236, smallY - meterBottom - 28);
    const auto largeY = meterBottom + 18;

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

    const std::array<std::pair<KnobComponent*, float>, 5> secondaryControls {{
        { &ratio, 0.17f },
        { &attack, 0.33f },
        { &release, 0.49f },
        { &mix, 0.65f },
        { &sidechainHighPass, 0.83f }
    }};

    for (const auto& [control, xRatio] : secondaryControls)
    {
        if (control == &sidechainHighPass)
        {
            if (audioProcessor.getTier() != PluginTier::Deluxe)
                continue;
        }

        const auto centreX = body.getX() + juce::roundToInt(static_cast<float>(body.getWidth()) * xRatio);
        control->setBounds(juce::Rectangle<int>(smallWidth, smallHeight).withCentre({ centreX, smallY + smallHeight / 2 }));
    }

    knee.setBounds(-1000, -1000, smallWidth, smallHeight);
}
void CompressorAudioProcessorEditor::timerCallback() { inputMeter.repaint(); outputMeter.repaint(); reductionMeter.repaint(); }
} // namespace compressor808bytes
