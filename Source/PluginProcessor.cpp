#include "PluginProcessor.h"
#include "UI/PluginEditor.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace compressor808bytes
{
namespace
{
juce::AudioParameterFloatAttributes parameterAttributes(const juce::String& label)
{
    return juce::AudioParameterFloatAttributes().withLabel(label);
}

float nearestFetRatio(float value) noexcept
{
    constexpr std::array<float, 5> ratios { 4.0f, 8.0f, 12.0f, 20.0f, 21.0f };
    auto nearest = ratios.front();
    auto nearestDistance = std::abs(value - nearest);

    for (const auto ratio : ratios)
    {
        const auto distance = std::abs(value - ratio);
        if (distance < nearestDistance)
        {
            nearest = ratio;
            nearestDistance = distance;
        }
    }

    return nearest;
}
} // namespace

CompressorAudioProcessor::CompressorAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout CompressorAudioProcessor::createParameterLayout()
{
    using Range = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> layout;
    const auto addFloat = [&layout] (const char* id, const char* name, Range range, float defaultValue, const char* label)
    {
        layout.push_back(std::make_unique<juce::AudioParameterFloat>(id, name, range, defaultValue, parameterAttributes(label)));
    };
    addFloat("input", "Input", { -24.0f, 24.0f, 0.1f }, 0.0f, "dB");
    addFloat("threshold", "Peak Reduction", { -60.0f, 0.0f, 0.1f }, -18.0f, "dB");
    addFloat("ratio", "Ratio", { 4.0f, 21.0f, 0.01f, 0.45f }, 4.0f, ":1");
    addFloat("attack", "Attack", { 0.02f, 0.8f, 0.001f, 0.45f }, 0.2f, "ms");
    addFloat("release", "Release", { 50.0f, 1100.0f, 0.1f, 0.45f }, 250.0f, "ms");
    addFloat("makeup", "Makeup", { -12.0f, 24.0f, 0.1f }, 0.0f, "dB");
    addFloat("mix", "Mix", { 0.0f, 100.0f, 0.1f }, 100.0f, "%");
    addFloat("output", "Output", { -24.0f, 12.0f, 0.1f }, 0.0f, "dB");
    addFloat("knee", "Knee", { 0.0f, 8.0f, 0.1f }, 1.5f, "dB");
    addFloat("sidechainHPF", "Sidechain HPF", { 20.0f, 500.0f, 1.0f, 0.35f }, 30.0f, "Hz");
    layout.push_back(std::make_unique<juce::AudioParameterChoice>("detectorMode", "Detector Mode", juce::StringArray { "Vintage", "Fast" }, 0));
    layout.push_back(std::make_unique<juce::AudioParameterBool>("autoGain", "Auto Gain", false));
    layout.push_back(std::make_unique<juce::AudioParameterChoice>("character", "Character", juce::StringArray { "Clean", "Transformer", "FET Push" }, 2));
    layout.push_back(std::make_unique<juce::AudioParameterChoice>("oversampling", "Oversampling", juce::StringArray { "Off", "2x", "4x" }, 1));
    layout.push_back(std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false));
    return { layout.begin(), layout.end() };
}

void CompressorAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    compressor.prepare(sampleRate, samplesPerBlock, getTotalNumInputChannels());
    gainReductionMeter.prepare(sampleRate);
    dryBuffer.setSize(getTotalNumInputChannels(), samplesPerBlock, false, false, true);
    for (auto* smoother : { &inputGainDb, &thresholdDb, &ratio, &attackMs, &releaseMs, &makeupDb, &mix, &outputGainDb, &kneeDb, &sidechainHighPassHz, &bypass })
        smoother->reset(sampleRate, 0.02);
    updateSmoothers();
}

void CompressorAudioProcessor::releaseResources()
{
    compressor.reset();
    gainReductionMeter.reset();
}

bool CompressorAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void CompressorAudioProcessor::updateSmoothers()
{
    const auto target = [this] (const char* id) { return parameters.getRawParameterValue(id)->load(); };
    inputGainDb.setTargetValue(target("input")); thresholdDb.setTargetValue(target("threshold")); ratio.setTargetValue(target("ratio"));
    attackMs.setTargetValue(target("attack")); releaseMs.setTargetValue(target("release")); makeupDb.setTargetValue(target("makeup"));
    mix.setTargetValue(target("mix")); outputGainDb.setTargetValue(target("output")); kneeDb.setTargetValue(target("knee"));
    sidechainHighPassHz.setTargetValue(target("sidechainHPF")); bypass.setTargetValue(target("bypass"));
}

float CompressorAudioProcessor::bufferPeakDb(const juce::AudioBuffer<float>& buffer) noexcept
{
    return juce::Decibels::gainToDecibels(std::max(buffer.getMagnitude(0, buffer.getNumSamples()), 1.0e-5f), -100.0f);
}

void CompressorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto channels = buffer.getNumChannels();
    const auto samples = buffer.getNumSamples();
    const auto parameterChoice = [this] (const char* id)
    {
        return static_cast<int>(std::round(parameters.getRawParameterValue(id)->load()));
    };
    inputLevelDb.store(bufferPeakDb(buffer), std::memory_order_relaxed);
    updateSmoothers();

    for (int sample = 0; sample < samples; ++sample)
    {
        const auto inputGain = juce::Decibels::decibelsToGain(inputGainDb.getNextValue());
        for (int channel = 0; channel < channels; ++channel)
            buffer.setSample(channel, sample, buffer.getSample(channel, sample) * inputGain);
    }

    for (int channel = 0; channel < channels; ++channel)
        dryBuffer.copyFrom(channel, 0, buffer, channel, 0, samples);
    auto blockGainReductionDb = 0.0f;
    for (int sample = 0; sample < samples; ++sample)
    {
        CompressorParameters current { thresholdDb.getNextValue(), nearestFetRatio(ratio.getNextValue()), attackMs.getNextValue(), releaseMs.getNextValue(), makeupDb.getNextValue(), kneeDb.getNextValue(), sidechainHighPassHz.getNextValue(), parameterChoice("detectorMode"), parameterChoice("character"), parameterChoice("oversampling") };
        compressor.setParameters(current);
        float* pointers[] { buffer.getWritePointer(0, sample), buffer.getWritePointer(1, sample) };
        compressor.process(pointers, channels, 1);
        const auto compressorGainReductionDb = compressor.getGainReductionDb();
        blockGainReductionDb = std::max(blockGainReductionDb, compressorGainReductionDb);
        const auto wet = mix.getNextValue() * 0.01f * (1.0f - bypass.getNextValue());
        const auto outputGainDbValue = outputGainDb.getNextValue();
        const auto outputGain = juce::Decibels::decibelsToGain(outputGainDbValue);
        const auto wetPathGain = juce::Decibels::decibelsToGain(current.makeupDb - compressorGainReductionDb);
        const auto blendedCompressorGain = 1.0f + wet * (wetPathGain - 1.0f);
        const auto signedMeterGainDb = juce::Decibels::gainToDecibels(std::max(blendedCompressorGain, 1.0e-5f), -100.0f) + outputGainDbValue;
        gainReductionMeter.processSample(signedMeterGainDb);
        for (int channel = 0; channel < channels; ++channel)
            buffer.setSample(channel, sample, (dryBuffer.getSample(channel, sample) + wet * (buffer.getSample(channel, sample) - dryBuffer.getSample(channel, sample))) * outputGain);
    }
    gainReductionDb.store(blockGainReductionDb, std::memory_order_relaxed);
    meterGainChangeDb.store(gainReductionMeter.getValueDb(), std::memory_order_relaxed);
    outputLevelDb.store(bufferPeakDb(buffer), std::memory_order_relaxed);
}

juce::AudioProcessorEditor* CompressorAudioProcessor::createEditor() { return new CompressorAudioProcessorEditor(*this); }
void CompressorAudioProcessor::getStateInformation(juce::MemoryBlock& destinationData)
{
    copyXmlToBinary(*parameters.copyState().createXml(), destinationData);
}
void CompressorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes); xml != nullptr && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}
} // namespace compressor808bytes

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new compressor808bytes::CompressorAudioProcessor();
}
