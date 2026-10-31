cmake_minimum_required(VERSION 3.22)
project(XPACEVERB VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(XPACEVERB
    COMPANY_NAME "TapeLab"
    PLUGIN_MANUFACTURER_CODE Tlab
    PLUGIN_CODE Xpcv
    FORMATS VST3 Standalone
    PRODUCT_NAME "XPACEVERB"
    VERSION 1.0.0
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx Reverb
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(XPACEVERB)

target_sources(XPACEVERB PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/ReverbEngine.cpp)

target_compile_definitions(XPACEVERB PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(XPACEVERB
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
