#include "PluginProcessor.h"

#ifndef COMPRESSOR808BYTES_PROCESSOR_TESTS
#include "UI/PluginEditor.h"
#endif

#include <algorithm>
#include <array>
#include <cmath>

namespace compressor808bytes
{
namespace
{
struct PresetParameterValue
{
    const char* id;
    float value;
};

struct FactoryPreset
{
    const char* name;
    const char* category;
    std::array<PresetParameterValue, 15> values;
};

constexpr std::array<FactoryPreset, 6> factoryPresets {{
    {
        "76 Start",
        "Utility",
        {{
            { "input", 0.0f }, { "threshold", -18.0f }, { "ratio", 4.0f }, { "attack", 0.20f }, { "release", 250.0f },
            { "makeup", 0.0f }, { "mix", 100.0f }, { "output", 0.0f }, { "knee", 1.5f }, { "sidechainHPF", 30.0f },
            { "detectorMode", 0.0f }, { "autoGain", 0.0f }, { "character", 1.0f }, { "oversampling", 1.0f }, { "bypass", 0.0f }
        }}
    },
    {
        "Vocal Grab",
        "Vocals",
        {{
            { "input", 6.0f }, { "threshold", -18.0f }, { "ratio", 4.0f }, { "attack", 0.12f }, { "release", 430.0f },
            { "makeup", 0.0f }, { "mix", 100.0f }, { "output", 0.0f }, { "knee", 2.0f }, { "sidechainHPF", 120.0f },
            { "detectorMode", 0.0f }, { "autoGain", 1.0f }, { "character", 1.0f }, { "oversampling", 1.0f }, { "bypass", 0.0f }
        }}
    },
    {
        "Bass Hold",
        "Bass",
        {{
            { "input", 5.0f }, { "threshold", -18.0f }, { "ratio", 8.0f }, { "attack", 0.28f }, { "release", 520.0f },
            { "makeup", 1.0f }, { "mix", 100.0f }, { "output", -1.0f }, { "knee", 2.5f }, { "sidechainHPF", 60.0f },
            { "detectorMode", 0.0f }, { "autoGain", 0.0f }, { "character", 1.0f }, { "oversampling", 1.0f }, { "bypass", 0.0f }
        }}
    },
    {
        "Drum Smash",
        "Drums",
        {{
            { "input", 13.0f }, { "threshold", -18.0f }, { "ratio", 21.0f }, { "attack", 0.72f }, { "release", 95.0f },
            { "makeup", -1.5f }, { "mix", 72.0f }, { "output", -2.0f }, { "knee", 0.4f }, { "sidechainHPF", 90.0f },
            { "detectorMode", 1.0f }, { "autoGain", 0.0f }, { "character", 2.0f }, { "oversampling", 2.0f }, { "bypass", 0.0f }
        }}
    },
    {
        "Parallel Snap",
        "Drums",
        {{
            { "input", 10.0f }, { "threshold", -18.0f }, { "ratio", 12.0f }, { "attack", 0.55f }, { "release", 140.0f },
            { "makeup", 1.5f }, { "mix", 48.0f }, { "output", -1.0f }, { "knee", 1.0f }, { "sidechainHPF", 110.0f },
            { "detectorMode", 1.0f }, { "autoGain", 1.0f }, { "character", 2.0f }, { "oversampling", 2.0f }, { "bypass", 0.0f }
        }}
    },
    {
        "Mix Kiss",
        "Bus",
        {{
            { "input", 2.0f }, { "threshold", -18.0f }, { "ratio", 4.0f }, { "attack", 0.45f }, { "release", 700.0f },
            { "makeup", 0.0f }, { "mix", 35.0f }, { "output", 0.0f }, { "knee", 3.0f }, { "sidechainHPF", 150.0f },
            { "detectorMode", 0.0f }, { "autoGain", 0.0f }, { "character", 1.0f }, { "oversampling", 1.0f }, { "bypass", 0.0f }
        }}
    }
}};

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

float internalFetThresholdDb(float legacyThresholdDb) noexcept
{
    constexpr auto nominalThresholdDb = -24.0f;
    const auto calibrationTrimDb = juce::jlimit(-4.0f, 4.0f, (legacyThresholdDb + 18.0f) * 0.16f);
    return nominalThresholdDb + calibrationTrimDb;
}

float automaticMakeupDb(float gainReductionDb) noexcept
{
    const auto reduction = juce::jlimit(0.0f, 24.0f, gainReductionDb);
    return juce::jlimit(0.0f, 12.0f, reduction * 0.55f);
}
} // namespace

CompressorAudioProcessor::CompressorAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

int CompressorAudioProcessor::getNumPrograms()
{
    return static_cast<int>(factoryPresets.size());
}

int CompressorAudioProcessor::getCurrentProgram()
{
    return currentProgram;
}

void CompressorAudioProcessor::setCurrentProgram(int index)
{
    applyFactoryPreset(index);
}

const juce::String CompressorAudioProcessor::getProgramName(int index)
{
    if (! juce::isPositiveAndBelow(index, getNumPrograms()))
        return {};

    return factoryPresets[static_cast<size_t>(index)].name;
}

juce::AudioProcessorValueTreeState::ParameterLayout CompressorAudioProcessor::createParameterLayout()
{
    using Range = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> layout;
    auto versionHint = 1;
    const auto addFloat = [&layout, &versionHint] (const char* id, const char* name, Range range, float defaultValue, const char* label)
    {
        layout.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { id, versionHint++ }, name, range, defaultValue, parameterAttributes(label)));
    };
    addFloat("input", "Input", { -24.0f, 24.0f, 0.1f }, 0.0f, "dB");
    addFloat("threshold", "FET Calibration", { -60.0f, 0.0f, 0.1f }, -18.0f, "dB");
    addFloat("ratio", "Ratio", { 4.0f, 21.0f, 0.01f, 0.45f }, 4.0f, ":1");
    addFloat("attack", "Attack", { 0.02f, 0.8f, 0.001f, 0.45f }, 0.2f, "ms");
    addFloat("release", "Release", { 50.0f, 1100.0f, 0.1f, 0.45f }, 250.0f, "ms");
    addFloat("makeup", "Output", { -24.0f, 24.0f, 0.1f }, 0.0f, "dB");
    addFloat("mix", "Mix", { 0.0f, 100.0f, 0.1f }, 100.0f, "%");
    addFloat("output", "Output Trim", { -24.0f, 12.0f, 0.1f }, 0.0f, "dB");
    addFloat("knee", "Knee", { 0.0f, 8.0f, 0.1f }, 1.5f, "dB");
    addFloat("sidechainHPF", "Sidechain HPF", { 20.0f, 500.0f, 1.0f, 0.35f }, 30.0f, "Hz");
    layout.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "detectorMode", versionHint++ }, "Detector Mode", juce::StringArray { "Vintage", "Fast" }, 0));
    layout.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "autoGain", versionHint++ }, "Auto Gain", false));
    layout.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "character", versionHint++ }, "Character", juce::StringArray { "Clean", "Transformer", "FET Push" }, 2));
    layout.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "oversampling", versionHint++ }, "Oversampling", juce::StringArray { "Off", "2x", "4x" }, 1));
    layout.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "bypass", versionHint++ }, "Bypass", false));
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

void CompressorAudioProcessor::sanitizeParameterState()
{
    const auto clampFloat = [this] (const char* id, float minimum, float maximum)
    {
        const auto rawValue = parameters.getRawParameterValue(id)->load();
        const auto clampedValue = juce::jlimit(minimum, maximum, rawValue);
        if (std::abs(rawValue - clampedValue) < 0.0001f)
            return;

        if (auto* parameter = parameters.getParameter(id))
            parameter->setValueNotifyingHost(parameter->convertTo0to1(clampedValue));
    };

    clampFloat("input", -24.0f, 24.0f);
    clampFloat("threshold", -60.0f, 0.0f);
    clampFloat("attack", 0.02f, 0.8f);
    clampFloat("release", 50.0f, 1100.0f);
    clampFloat("makeup", -24.0f, 24.0f);
    clampFloat("mix", 0.0f, 100.0f);
    clampFloat("output", -24.0f, 12.0f);
    clampFloat("knee", 0.0f, 8.0f);
    clampFloat("sidechainHPF", 20.0f, 500.0f);

    if (auto* ratioParameter = parameters.getParameter("ratio"))
    {
        const auto snappedRatio = nearestFetRatio(parameters.getRawParameterValue("ratio")->load());
        ratioParameter->setValueNotifyingHost(ratioParameter->convertTo0to1(snappedRatio));
    }
}

void CompressorAudioProcessor::applyFactoryPreset(int index)
{
    currentProgram = juce::jlimit(0, getNumPrograms() - 1, index);
    const auto& preset = factoryPresets[static_cast<size_t>(currentProgram)];

    for (const auto& value : preset.values)
    {
        if (auto* parameter = parameters.getParameter(value.id))
        {
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost(parameter->convertTo0to1(value.value));
            parameter->endChangeGesture();
        }
    }

    sanitizeParameterState();
    updateSmoothers();
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
    const auto autoGainEnabled = parameters.getRawParameterValue("autoGain")->load() >= 0.5f;

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
        CompressorParameters current { internalFetThresholdDb(thresholdDb.getNextValue()), nearestFetRatio(ratio.getNextValue()), attackMs.getNextValue(), releaseMs.getNextValue(), makeupDb.getNextValue(), kneeDb.getNextValue(), sidechainHighPassHz.getNextValue(), parameterChoice("detectorMode"), parameterChoice("character"), parameterChoice("oversampling") };
        compressor.setParameters(current);
        float* pointers[] { buffer.getWritePointer(0, sample), buffer.getWritePointer(1, sample) };
        compressor.process(pointers, channels, 1);
        const auto compressorGainReductionDb = compressor.getGainReductionDb();
        blockGainReductionDb = std::max(blockGainReductionDb, compressorGainReductionDb);
        const auto wet = mix.getNextValue() * 0.01f * (1.0f - bypass.getNextValue());
        const auto autoMakeupDb = autoGainEnabled ? automaticMakeupDb(compressorGainReductionDb) : 0.0f;
        const auto autoMakeupGain = juce::Decibels::decibelsToGain(autoMakeupDb);
        const auto outputGainDbValue = outputGainDb.getNextValue();
        const auto outputGain = juce::Decibels::decibelsToGain(outputGainDbValue);
        const auto wetPathGain = juce::Decibels::decibelsToGain(current.makeupDb + autoMakeupDb - compressorGainReductionDb);
        const auto blendedCompressorGain = 1.0f + wet * (wetPathGain - 1.0f);
        const auto signedMeterGainDb = juce::Decibels::gainToDecibels(std::max(blendedCompressorGain, 1.0e-5f), -100.0f) + outputGainDbValue;
        gainReductionMeter.processSample(signedMeterGainDb);
        for (int channel = 0; channel < channels; ++channel)
        {
            const auto dry = dryBuffer.getSample(channel, sample);
            const auto compensatedWet = buffer.getSample(channel, sample) * autoMakeupGain;
            buffer.setSample(channel, sample, (dry + wet * (compensatedWet - dry)) * outputGain);
        }
    }
    gainReductionDb.store(blockGainReductionDb, std::memory_order_relaxed);
    meterGainChangeDb.store(gainReductionMeter.getValueDb(), std::memory_order_relaxed);
    outputLevelDb.store(bufferPeakDb(buffer), std::memory_order_relaxed);
}

juce::AudioProcessorEditor* CompressorAudioProcessor::createEditor()
{
#ifdef COMPRESSOR808BYTES_PROCESSOR_TESTS
    return nullptr;
#else
    return new CompressorAudioProcessorEditor(*this);
#endif
}
void CompressorAudioProcessor::getStateInformation(juce::MemoryBlock& destinationData)
{
    auto state = parameters.copyState();
    state.setProperty("currentProgram", currentProgram, nullptr);
    copyXmlToBinary(*state.createXml(), destinationData);
}
void CompressorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes); xml != nullptr && xml->hasTagName(parameters.state.getType()))
    {
        auto state = juce::ValueTree::fromXml(*xml);
        if (state.hasProperty("currentProgram"))
            currentProgram = juce::jlimit(0, getNumPrograms() - 1, static_cast<int>(state.getProperty("currentProgram")));

        parameters.replaceState(state);
        sanitizeParameterState();
        updateSmoothers();
    }
}
} // namespace compressor808bytes

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new compressor808bytes::CompressorAudioProcessor();
}
