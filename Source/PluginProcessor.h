#pragma once

#include "DSP/CompressorEngine.h"
#include "DSP/MeterBallistics.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace compressor808bytes
{
enum class PluginTier { Lite, Deluxe };

#if COMPRESSOR808BYTES_DELUXE
constexpr PluginTier buildTier = PluginTier::Deluxe;
#else
constexpr PluginTier buildTier = PluginTier::Lite;
#endif

class CompressorAudioProcessor final : public juce::AudioProcessor
{
public:
    CompressorAudioProcessor();
    ~CompressorAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& destinationData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    [[nodiscard]] static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    [[nodiscard]] float getInputLevelDb() const noexcept { return inputLevelDb.load(); }
    [[nodiscard]] float getOutputLevelDb() const noexcept { return outputLevelDb.load(); }
    [[nodiscard]] float getGainReductionDb() const noexcept { return gainReductionDb.load(); }
    [[nodiscard]] float getMeterGainChangeDb() const noexcept { return meterGainChangeDb.load(); }
    [[nodiscard]] const std::atomic<float>& getInputLevelSource() const noexcept { return inputLevelDb; }
    [[nodiscard]] const std::atomic<float>& getOutputLevelSource() const noexcept { return outputLevelDb; }
    [[nodiscard]] const std::atomic<float>& getGainReductionSource() const noexcept { return gainReductionDb; }
    [[nodiscard]] const std::atomic<float>& getMeterGainChangeSource() const noexcept { return meterGainChangeDb; }
    [[nodiscard]] PluginTier getTier() const noexcept { return buildTier; }

    juce::AudioProcessorValueTreeState parameters;

private:
    void updateSmoothers();
    static float bufferPeakDb(const juce::AudioBuffer<float>& buffer) noexcept;

    CompressorEngine compressor;
    GainReductionMeterBallistics gainReductionMeter;
    juce::AudioBuffer<float> dryBuffer;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inputGainDb, thresholdDb, ratio;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> attackMs, releaseMs, makeupDb, mix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGainDb, kneeDb, sidechainHighPassHz, bypass;
    std::atomic<float> inputLevelDb { -100.0f }, outputLevelDb { -100.0f }, gainReductionDb { 0.0f }, meterGainChangeDb { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorAudioProcessor)
};
} // namespace compressor808bytes
