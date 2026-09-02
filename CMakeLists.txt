cmake_minimum_required(VERSION 3.22)

project(Compressor808Bytes VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(juce)

option(COMPRESSOR808BYTES_DELUXE "Build with Deluxe controls enabled" ON)
option(COMPRESSOR808BYTES_BUILD_TESTS "Build DSP tests" ON)

juce_add_plugin(Compressor808Bytes
    COMPANY_NAME "808Bytes"
    BUNDLE_ID "com.808bytes.compressor"
    PLUGIN_MANUFACTURER_CODE Byts
    PLUGIN_CODE C808
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "808Bytes Compressor"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(Compressor808Bytes PRIVATE
    Source/PluginProcessor.cpp
    Source/UI/PluginEditor.cpp
    Source/UI/WeatheredLookAndFeel.cpp
    Source/UI/KnobComponent.cpp
    Source/UI/MeterComponent.cpp
    Source/DSP/CompressorEngine.cpp
    Source/DSP/EnvelopeDetector.cpp
    Source/DSP/Filters.cpp)

target_compile_features(Compressor808Bytes PRIVATE cxx_std_20)
target_compile_definitions(Compressor808Bytes PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    $<$<BOOL:${COMPRESSOR808BYTES_DELUXE}>:COMPRESSOR808BYTES_DELUXE=1>)
target_link_libraries(Compressor808Bytes PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

if (COMPRESSOR808BYTES_BUILD_TESTS)
    enable_testing()
    add_executable(CompressorDSPTests
        tests/CompressorEngineTests.cpp
        Source/DSP/CompressorEngine.cpp
        Source/DSP/EnvelopeDetector.cpp
        Source/DSP/Filters.cpp)
    target_compile_features(CompressorDSPTests PRIVATE cxx_std_20)
    target_include_directories(CompressorDSPTests PRIVATE Source)
    target_link_libraries(CompressorDSPTests PRIVATE juce::juce_audio_basics juce::juce_dsp)
    add_test(NAME CompressorDSPTests COMMAND CompressorDSPTests)
endif()