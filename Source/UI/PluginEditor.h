#pragma once

#include "KnobComponent.h"
#include "MeterComponent.h"
#include "WeatheredLookAndFeel.h"
#include "../PluginProcessor.h"

#include <array>

namespace compressor808bytes
{
class CompressorAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit CompressorAudioProcessorEditor(CompressorAudioProcessor&);
    ~CompressorAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void applyControlHierarchy();
    void configureChoiceControl(juce::Label&, juce::ComboBox&, const juce::String& labelText, const juce::StringArray& items);
    void configureRatioButton(juce::TextButton&, const juce::String& text, float ratioValue);
    void layoutChoiceControl(juce::Label&, juce::ComboBox&, juce::Rectangle<int>);
    void layoutRatioButtons(juce::Rectangle<int> bounds);
    void layoutMeterModeButtons(juce::Rectangle<int> bounds);
    void setRatioFromButton(float ratioValue);
    void updateRatioButtons();
    void setMeterMode(MeterMode newMode);
    void updateMeterModeButtons();
    void timerCallback() override;
    CompressorAudioProcessor& audioProcessor;
    WeatheredLookAndFeel weatheredLookAndFeel;
    KnobComponent input { "Input", "dB" }, threshold { "Cal", "dB" }, attack { "Attack", "ms" }, release { "Release", "ms" }, makeup { "Output", "dB" }, mix { "Mix", "%" }, output { "Trim", "dB" }, knee { "Knee", "dB" }, sidechainHighPass { "SC HPF", "Hz" };
    MeterComponent inputMeter, outputMeter, reductionMeter;
    juce::ToggleButton bypass { "Bypass" };
    juce::Label ratioLabel;
    std::array<juce::TextButton, 5> ratioButtons;
    std::array<juce::TextButton, 3> meterModeButtons;
    MeterMode meterMode { MeterMode::GainReduction };
    juce::Label detectorModeLabel, characterLabel, oversamplingLabel;
    juce::ComboBox detectorMode, character, oversampling;
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<Attachment> inputAttachment, thresholdAttachment, attackAttachment, releaseAttachment, makeupAttachment, mixAttachment, outputAttachment, kneeAttachment, highPassAttachment;
    std::unique_ptr<ComboAttachment> detectorModeAttachment, characterAttachment, oversamplingAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorAudioProcessorEditor)
};
} // namespace compressor808bytes
