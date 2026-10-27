cmake_minimum_required(VERSION 3.22)
project(AuraCompressor VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_plugin(AuraCompressor
    COMPANY_NAME "AuraDSP"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    PLUGIN_NAME "Aura Compressor"
    PRODUCT_NAME "AuraCompressor"
    FORMATS VST3 Standalone LV2
    PLUGIN_CODE Acom
    PLUGIN_MANUFACTURER_CODE Aura
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(AuraCompressor
    PRIVATE
        Source/PluginProcessor.h
        Source/PluginProcessor.cpp
        Source/PluginEditor.h
        Source/PluginEditor.cpp
        Source/Audio/SidechainFilter.h
        Source/Audio/CompressorDSP.h
        Source/UI/CustomLookAndFeel.h
        Source/UI/CustomLookAndFeel.cpp
        Source/UI/RotaryKnob.h
        Source/UI/MeterComponent.h
        Source/UI/TransferCurveComponent.h
)

target_compile_definitions(AuraCompressor
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(AuraCompressor
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
