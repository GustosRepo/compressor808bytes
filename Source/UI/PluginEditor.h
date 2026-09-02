#pragma once

#include "KnobComponent.h"
#include "MeterComponent.h"
#include "WeatheredLookAndFeel.h"
#include "../PluginProcessor.h"

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
    void layoutChoiceControl(juce::Label&, juce::ComboBox&, juce::Rectangle<int>);
    void timerCallback() override;
    CompressorAudioProcessor& audioProcessor;
    WeatheredLookAndFeel weatheredLookAndFeel;
    KnobComponent input { "Input", "dB" }, threshold { "Peak Reduction", "dB" }, ratio { "Ratio", ":1" }, attack { "Attack", "ms" }, release { "Release", "ms" }, makeup { "Gain", "dB" }, mix { "Mix", "%" }, output { "Output", "dB" }, knee { "Knee", "dB" }, sidechainHighPass { "SC HPF", "Hz" };
    MeterComponent inputMeter, outputMeter, reductionMeter;
    juce::ToggleButton bypass { "Bypass" };
    juce::Label detectorModeLabel, characterLabel, oversamplingLabel;
    juce::ComboBox detectorMode, character, oversampling;
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<Attachment> inputAttachment, thresholdAttachment, ratioAttachment, attackAttachment, releaseAttachment, makeupAttachment, mixAttachment, outputAttachment, kneeAttachment, highPassAttachment;
    std::unique_ptr<ComboAttachment> detectorModeAttachment, characterAttachment, oversamplingAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorAudioProcessorEditor)
};
} // namespace compressor808bytes
